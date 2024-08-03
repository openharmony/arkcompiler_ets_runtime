/*
 * Copyright (c) 2024 Huawei Device Co., Ltd.
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef NATIVE_PROF_DUMP_JSON_H
#define NATIVE_PROF_DUMP_JSON_H

#include <iostream>

#define PROF_DUMP_JSON_API DUMP_PUBLIC_API
#ifndef PROF_DUMP_JSON_WINDOWS
#define DUMP_PUBLIC_API __attribute__((visibility ("default")))
#else
#define DUMP_PUBLIC_API __declspec(dllexport)
#endif

namespace native {
extern "C" {
    PROF_DUMP_JSON_API size_t ConvertApToJson(const char *path, const size_t pathSize);
    PROF_DUMP_JSON_API bool GetConvertResult(char *buffer, size_t length);
}
} // namespace native
#endif // NATIVE_PROF_DUMP_JSON_H
