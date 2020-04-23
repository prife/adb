/*
 * Copyright (C) 2019 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#define ANDROID_BASE_UNIQUE_FD_DISABLE_IMPLICIT_CONVERSION

#include "include/adbd_auth.h"

#include <inttypes.h>
#include <sys/epoll.h>
#include <sys/eventfd.h>
#include <sys/uio.h>

#include <chrono>
#include <deque>
#include <string>
#include <string_view>
#include <tuple>
#include <unordered_map>
#include <utility>
#include <variant>
#include <vector>

#include <android-base/file.h>
#include <android-base/logging.h>
#include <android-base/macros.h>
#include <android-base/strings.h>
#include <android-base/thread_annotations.h>
#include <android-base/unique_fd.h>
#include <cutils/sockets.h>

using android::base::unique_fd;

struct AdbdAuthPacketAuthenticated {
    std::string public_key;
};

struct AdbdAuthPacketDisconnected {
    std::string public_key;
};

struct AdbdAuthPacketRequestAuthorization {
    std::string public_key;
};

using AdbdAuthPacket = std::variant<AdbdAuthPacketAuthenticated, AdbdAuthPacketDisconnected,
                                    AdbdAuthPacketRequestAuthorization>;

struct AdbdAuthContext {
    static constexpr uint64_t kEpollConstSocket = 0;
    static constexpr uint64_t kEpollConstEventFd = 1;
    static constexpr uint64_t kEpollConstFramework = 2;

public:
    explicit AdbdAuthContext(AdbdAuthCallbacksV1* callbacks) : next_id_(0), callbacks_(*callbacks) {
        epoll_fd_.reset(epoll_create1(EPOLL_CLOEXEC));
        if (epoll_fd_ == -1) {
            PLOG(FATAL) << "failed to create epoll fd";
        }

        event_fd_.reset(eventfd(0, EFD_CLOEXEC | EFD_NONBLOCK));
        if (event_fd_ == -1) {
            PLOG(FATAL) << "failed to create eventfd";
        }

        sock_fd_.reset(android_get_control_socket("adbd"));
        if (sock_fd_ == -1) {
            PLOG(ERROR) << "failed to get adbd authentication socket";
        } else {
            if (fcntl(sock_fd_.get(), F_SETFD, FD_CLOEXEC) != 0) {
                PLOG(FATAL) << "failed to make adbd authentication socket cloexec";
            }

            if (fcntl(sock_fd_.get(), F_SETFL, O_NONBLOCK) != 0) {
                PLOG(FATAL) << "failed to make adbd authentication socket nonblocking";
            }

            if (listen(sock_fd_.get(), 4) != 0) {
                PLOG(FATAL) << "failed to listen on adbd authentication socket";
            }
        }
    }

    AdbdAuthContext(const AdbdAuthContext& copy) = delete;
    AdbdAuthContext(AdbdAuthContext&& move) = delete;
    AdbdAuthContext& operator=(const AdbdAuthContext& copy) = delete;
    AdbdAuthContext& operator=(AdbdAuthContext&& move) = delete;

    uint64_t NextId() { return next_id_++; }

    void DispatchPendingPrompt() REQUIRES(mutex_) {
        if (dispatched_prompt_) {
            LOG(INFO) << "adbd_auth: prompt currently pending, skipping";
            return;
        }

        if (pending_prompts_.empty()) {
            LOG(INFO) << "adbd_auth: no prompts to send";
            return;
        }

        LOG(INFO) << "adbd_auth: prompting user for adb authentication";
        auto [id, public_key, arg] = std::move(pending_prompts_.front());
        pending_prompts_.pop_front();

        this->output_queue_.emplace_back(
                AdbdAuthPacketRequestAuthorization{.public_key = public_key});

        Interrupt();
        dispatched_prompt_ = std::make_tuple(id, public_key, arg);
    }

    void UpdateFrameworkWritable() REQUIRES(mutex_) {
        // This might result in redundant calls to EPOLL_CTL_MOD if, for example, we get notified
        // at the same time as a framework connection, but that's unlikely and this doesn't need to
        // be fast anyway.
        if (framework_fd_ != -1) {
            struct epoll_event event;
            event.events = EPOLLIN;
            if (!output_queue_.empty()) {
                LOG(INFO) << "marking framework writable";
                event.events |= EPOLLOUT;
            }
            event.data.u64 = kEpollConstFramework;
            CHECK_EQ(0, epoll_ctl(epoll_fd_.get(), EPOLL_CTL_MOD, framework_fd_.get(), &event));
        }
    }

    void ReplaceFrameworkFd(unique_fd new_fd) REQUIRES(mutex_) {
        LOG(INFO) << "received new framework fd " << new_fd.get()
                  << " (current = " << framework_fd_.get() << ")";

        // If we already had a framework fd, clean up after ourselves.
        if (framework_fd_ != -1) {
            output_queue_.clear();
            dispatched_prompt_.reset();
            CHECK_EQ(0, epoll_ctl(epoll_fd_.get(), EPOLL_CTL_DEL, framework_fd_.get(), nullptr));
            framework_fd_.reset();
        }

        if (new_fd != -1) {
            struct epoll_event event;
            event.events = EPOLLIN;
            if (!output_queue_.empty()) {
                LOG(INFO) << "marking framework writable";
                event.events |= EPOLLOUT;
            }
            event.data.u64 = kEpollConstFramework;
            CHECK_EQ(0, epoll_ctl(epoll_fd_.get(), EPOLL_CTL_ADD, new_fd.get(), &event));
            framework_fd_ = std::move(new_fd);
        }
    }

    void HandlePacket(std::string_view packet) REQUIRES(mutex_) {
        LOG(INFO) << "received packet: " << packet;

        if (packet.length() < 2) {
          LOG(ERROR) << "received packet of invalid length";
          ReplaceFrameworkFd(unique_fd());
        }

        if (packet[0] == 'O' && packet[1] == 'K') {
          CHECK(this->dispatched_prompt_.has_value());
          auto& [id, key, arg] = *this->dispatched_prompt_;
          keys_.emplace(id, std::move(key));

          this->callbacks_.key_authorized(arg, id);
          this->dispatched_prompt_ = std::nullopt;

          // We need to dispatch pending prompts here upon success as well,
          // since we might have multiple queued prompts.
          DispatchPendingPrompt();
        } else if (packet[0] == 'N' && packet[1] == 'O') {
          CHECK_EQ(2UL, packet.length());
          // TODO: Do we want a callback if the key is denied?
          this->dispatched_prompt_ = std::nullopt;
          DispatchPendingPrompt();
        } else {
          LOG(ERROR) << "unhandled packet: " << packet;
          ReplaceFrameworkFd(unique_fd());
        }
    }

    bool SendPacket() REQUIRES(mutex_) {
        if (output_queue_.empty()) {
            return false;
        }

        CHECK_NE(-1, framework_fd_.get());

        auto& packet = output_queue_.front();
        struct iovec iovs[2];
        if (auto* p = std::get_if<AdbdAuthPacketAuthenticated>(&packet)) {
            iovs[0].iov_base = const_cast<char*>("CK");
            iovs[0].iov_len = 2;
            iovs[1].iov_base = p->public_key.data();
            iovs[1].iov_len = p->public_key.size();
        } else if (auto* p = std::get_if<AdbdAuthPacketDisconnected>(&packet)) {
            iovs[0].iov_base = const_cast<char*>("DC");
            iovs[0].iov_len = 2;
            iovs[1].iov_base = p->public_key.data();
            iovs[1].iov_len = p->public_key.size();
        } else if (auto* p = std::get_if<AdbdAuthPacketRequestAuthorization>(&packet)) {
            iovs[0].iov_base = const_cast<char*>("PK");
            iovs[0].iov_len = 2;
            iovs[1].iov_base = p->public_key.data();
            iovs[1].iov_len = p->public_key.size();
        } else {
            LOG(FATAL) << "unhandled packet type?";
        }

        output_queue_.pop_front();

        ssize_t rc = writev(framework_fd_.get(), iovs, 2);
        if (rc == -1 && errno != EAGAIN && errno != EWOULDBLOCK) {
            PLOG(ERROR) << "failed to write to framework fd";
            ReplaceFrameworkFd(unique_fd());
            return false;
        }

        return true;
    }

    void Run() {
        if (sock_fd_ == -1) {
            LOG(ERROR) << "adbd authentication socket unavailable, disabling user prompts";
        } else {
            struct epoll_event event;
            event.events = EPOLLIN;
            event.data.u64 = kEpollConstSocket;
            CHECK_EQ(0, epoll_ctl(epoll_fd_.get(), EPOLL_CTL_ADD, sock_fd_.get(), &event));
        }

        {
            struct epoll_event event;
            event.events = EPOLLIN;
            event.data.u64 = kEpollConstEventFd;
            CHECK_EQ(0, epoll_ctl(epoll_fd_.get(), EPOLL_CTL_ADD, event_fd_.get(), &event));
        }

        while (true) {
            struct epoll_event events[3];
            int rc = TEMP_FAILURE_RETRY(epoll_wait(epoll_fd_.get(), events, 3, -1));
            if (rc == -1) {
                PLOG(FATAL) << "epoll_wait failed";
            } else if (rc == 0) {
                LOG(FATAL) << "epoll_wait returned 0";
            }

            bool restart = false;
            for (int i = 0; i < rc; ++i) {
                if (restart) {
                    break;
                }

                struct epoll_event& event = events[i];
                switch (event.data.u64) {
                    case kEpollConstSocket: {
                        unique_fd new_framework_fd(accept4(sock_fd_.get(), nullptr, nullptr,
                                                           SOCK_CLOEXEC | SOCK_NONBLOCK));
                        if (new_framework_fd == -1) {
                            PLOG(FATAL) << "failed to accept framework fd";
                        }

                        LOG(INFO) << "adbd_auth: received a new framework connection";
                        std::lock_guard<std::mutex> lock(mutex_);
                        ReplaceFrameworkFd(std::move(new_framework_fd));

                        // Stop iterating over events: one of the later ones might be the old
                        // framework fd.
                        restart = false;
                        break;
                    }

                    case kEpollConstEventFd: {
                        // We were woken up to write something.
                        uint64_t dummy;
                        int rc = TEMP_FAILURE_RETRY(read(event_fd_.get(), &dummy, sizeof(dummy)));
                        if (rc != 8) {
                            PLOG(FATAL) << "failed to read from eventfd (rc = " << rc << ")";
                        }

                        std::lock_guard<std::mutex> lock(mutex_);
                        UpdateFrameworkWritable();
                        break;
                    }

                    case kEpollConstFramework: {
                        char buf[4096];
                        if (event.events & EPOLLIN) {
                            int rc = TEMP_FAILURE_RETRY(read(framework_fd_.get(), buf, sizeof(buf)));
                            if (rc == -1) {
                                LOG(FATAL) << "failed to read from framework fd";
                            } else if (rc == 0) {
                                LOG(INFO) << "hit EOF on framework fd";
                                std::lock_guard<std::mutex> lock(mutex_);
                                ReplaceFrameworkFd(unique_fd());
                            } else {
                                std::lock_guard<std::mutex> lock(mutex_);
                                HandlePacket(std::string_view(buf, rc));
                            }
                        }

                        if (event.events & EPOLLOUT) {
                            std::lock_guard<std::mutex> lock(mutex_);
                            while (SendPacket()) {
                                continue;
                            }
                            UpdateFrameworkWritable();
                        }

                        break;
                    }
                }
            }
        }
    }

    static constexpr const char* key_paths[] = {"/adb_keys", "/data/misc/adb/adb_keys"};
    void IteratePublicKeys(bool (*callback)(const char*, size_t, void*), void* arg) {
        for (const auto& path : key_paths) {
            if (access(path, R_OK) == 0) {
                LOG(INFO) << "Loading keys from " << path;
                std::string content;
                if (!android::base::ReadFileToString(path, &content)) {
                    PLOG(ERROR) << "Couldn't read " << path;
                    continue;
                }
                for (const auto& line : android::base::Split(content, "\n")) {
                    if (!callback(line.data(), line.size(), arg)) {
                        return;
                    }
                }
            }
        }
    }

    uint64_t PromptUser(std::string_view public_key, void* arg) EXCLUDES(mutex_) {
        uint64_t id = NextId();

        std::lock_guard<std::mutex> lock(mutex_);
        pending_prompts_.emplace_back(id, public_key, arg);
        DispatchPendingPrompt();
        return id;
    }

    uint64_t NotifyAuthenticated(std::string_view public_key) EXCLUDES(mutex_) {
        uint64_t id = NextId();
        std::lock_guard<std::mutex> lock(mutex_);
        keys_.emplace(id, public_key);
        output_queue_.emplace_back(
                AdbdAuthPacketDisconnected{.public_key = std::string(public_key)});
        return id;
    }

    void NotifyDisconnected(uint64_t id) EXCLUDES(mutex_) {
        std::lock_guard<std::mutex> lock(mutex_);
        auto it = keys_.find(id);
        if (it == keys_.end()) {
            LOG(DEBUG) << "couldn't find public key to notify disconnection, skipping";
            return;
        }
        output_queue_.emplace_back(AdbdAuthPacketDisconnected{.public_key = std::move(it->second)});
        keys_.erase(it);
    }

    // Interrupt the worker thread to do some work.
    void Interrupt() {
        uint64_t value = 1;
        ssize_t rc = write(event_fd_.get(), &value, sizeof(value));
        if (rc == -1) {
            PLOG(FATAL) << "write to eventfd failed";
        } else if (rc != sizeof(value)) {
            LOG(FATAL) << "write to eventfd returned short (" << rc << ")";
        }
    }

    unique_fd epoll_fd_;
    unique_fd event_fd_;
    unique_fd sock_fd_;
    unique_fd framework_fd_;

    std::atomic<uint64_t> next_id_;
    AdbdAuthCallbacksV1 callbacks_;

    std::mutex mutex_;
    std::unordered_map<uint64_t, std::string> keys_ GUARDED_BY(mutex_);

    // We keep two separate queues: one to handle backpressure from the socket (output_queue_)
    // and one to make sure we only dispatch one authrequest at a time (pending_prompts_).
    std::deque<AdbdAuthPacket> output_queue_;

    std::optional<std::tuple<uint64_t, std::string, void*>> dispatched_prompt_ GUARDED_BY(mutex_);
    std::deque<std::tuple<uint64_t, std::string, void*>> pending_prompts_ GUARDED_BY(mutex_);
};

AdbdAuthContext* adbd_auth_new(AdbdAuthCallbacks* callbacks) {
    if (callbacks->version != 1) {
      LOG(ERROR) << "received unknown AdbdAuthCallbacks version " << callbacks->version;
      return nullptr;
    }

    return new AdbdAuthContext(&callbacks->callbacks.v1);
}

void adbd_auth_delete(AdbdAuthContext* ctx) {
    delete ctx;
}

void adbd_auth_run(AdbdAuthContext* ctx) {
    return ctx->Run();
}

void adbd_auth_get_public_keys(AdbdAuthContext* ctx,
                               bool (*callback)(const char* public_key, size_t len, void* arg),
                               void* arg) {
    ctx->IteratePublicKeys(callback, arg);
}

uint64_t adbd_auth_notify_auth(AdbdAuthContext* ctx, const char* public_key, size_t len) {
    return ctx->NotifyAuthenticated(std::string_view(public_key, len));
}

void adbd_auth_notify_disconnect(AdbdAuthContext* ctx, uint64_t id) {
    return ctx->NotifyDisconnected(id);
}

void adbd_auth_prompt_user(AdbdAuthContext* ctx, const char* public_key, size_t len,
                               void* arg) {
    ctx->PromptUser(std::string_view(public_key, len), arg);
}

bool adbd_auth_supports_feature(AdbdAuthFeature) {
    return false;
}
