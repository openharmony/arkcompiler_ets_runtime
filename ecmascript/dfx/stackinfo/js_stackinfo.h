/*
 * Copyright (c) 2022 Huawei Device Co., Ltd.
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

#ifndef ECMASCRIPT_DFX_STACKINFO_JS_STACKINFO_H
#define ECMASCRIPT_DFX_STACKINFO_JS_STACKINFO_H

#include <csignal>
#include "ecmascript/compiler/aot_file/aot_file_manager.h"
#include "ecmascript/js_thread.h"

namespace panda::ecmascript {
struct JsFrameInfo {
    std::string functionName;
    std::string fileName;
    std::string pos;
    uintptr_t *nativePointer = nullptr;
};
class JsStackInfo {
public:
    static std::string BuildJsStackTrace(JSThread *thread, bool needNative);
    static std::vector<JsFrameInfo> BuildJsStackInfo(JSThread *thread);
    static std::string BuildMethodTrace(Method *method, uint32_t pcOffset);
    static AOTFileManager *loader;
};
void CrashCallback(char *buf, size_t len, void *ucontext);
} // namespace panda::ecmascript
#endif  // ECMASCRIPT_DFX_STACKINFO_JS_STACKINFO_H
extern "C" int step_ark_managed_native_frame(
    int pid, uintptr_t *pc, uintptr_t *fp, uintptr_t *sp, char *buf, size_t buf_sz);
extern "C" int get_ark_js_heap_crash_info(
    int pid, uintptr_t *x20, uintptr_t *fp, int out_js_info, char *buf, size_t buf_sz);
// define in dfx_signal_handler.h
typedef void(*ThreadInfoCallback)(char *buf, size_t len, void *ucontext);
extern "C" void SetThreadInfoCallback(ThreadInfoCallback func) __attribute__((weak));