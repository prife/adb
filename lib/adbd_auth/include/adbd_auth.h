#pragma once

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

#include <stdbool.h>
#include <stdint.h>
#include <sys/types.h>

extern "C" {

struct AdbdAuthCallbacksV1 {
    // Callback for a successful user authorization.
    void (*key_authorized)(void* arg, uint64_t id);
};

struct AdbdAuthCallbacks {
    uint32_t version;
    union {
        AdbdAuthCallbacksV1 v1;
    } callbacks;
};

struct AdbdAuthContext;

AdbdAuthContext* adbd_auth_new(AdbdAuthCallbacks* callbacks);
void adbd_auth_delete(AdbdAuthContext* ctx);

void adbd_auth_run(AdbdAuthContext* ctx);

// Iterate through the list of authorized public keys.
// Return false from the callback to stop iteration.
void adbd_auth_get_public_keys(AdbdAuthContext* ctx,
                               bool (*callback)(const char* public_key, size_t len, void* arg),
                               void* arg);

// Let system_server know that a key has been successfully used for authentication.
uint64_t adbd_auth_notify_auth(AdbdAuthContext* ctx, const char* public_key, size_t len);

// Let system_server know that a connection has been closed.
void adbd_auth_notify_disconnect(AdbdAuthContext* ctx, uint64_t id);

// Prompt the user to authorize a public key.
// When this happens, a callback will be run on the auth thread with the result.
void adbd_auth_prompt_user(AdbdAuthContext* ctx, const char* public_key, size_t len, void* arg);

enum AdbdAuthFeature {
};

bool adbd_auth_supports_feature(AdbdAuthFeature f);

}
