/*
 * Copyright (c) 2021 Huawei Device Co., Ltd.
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

#include <csignal>

#include "ecmascript/base/string_helper.h"
#include "ecmascript/ecma_vm.h"
#include "ecmascript/mem/clock_scope.h"
#include "ecmascript/platform/os.h"


namespace panda::ecmascript {
void BlockSignals()
{
#if defined(PANDA_TARGET_UNIX)
    sigset_t set;
    if (sigemptyset(&set) == -1) {
        LOG_ECMA(ERROR) << "sigemptyset failed";
        return;
    }
#endif  // PANDA_TARGET_UNIX
}

std::string GetHelper()
{
    std::string str;
    str.append(COMMON_HELP_HEAD_MSG);
    str.append(HELP_OPTION_MSG);
    return str;
}

static bool g_testEnd = false;

bool IsEqual(EcmaVM *vm, Local<JSValueRef> jsArg0, Local<JSValueRef> jsArg1)
{
    if (jsArg0->IsStrictEquals(vm, jsArg1)) {
        return true;
    } else if ((jsArg0->IsJSArray(vm) && jsArg1->IsJSArray(vm))) {
        Local<ArrayRef> arr0(jsArg0);
        Local<ArrayRef> arr1(jsArg1);
        uint32_t length = arr0->Length(vm);
        if (length != arr1->Length(vm)) {
            return false;
        }
        for (uint32_t i = 0; i < length; i++) {
            Local<JSValueRef> arg0 = ArrayRef::GetValueAt(vm, arr0, i);
            Local<JSValueRef> arg1 = ArrayRef::GetValueAt(vm, arr1, i);
            if (!IsEqual(vm, arg0, arg1)) {
                return false;
            }
        }
        return true;
    }
    return false;
}

Local<JSValueRef> TestEnd(JsiRuntimeCallInfo *runtimeInfo)
{
    EcmaVM *vm = runtimeInfo->GetVM();
    g_testEnd = true;
    return JSValueRef::Undefined(vm);
}

Local<JSValueRef> AssertEqual(JsiRuntimeCallInfo *runtimeInfo)
{
    EcmaVM *vm = runtimeInfo->GetVM();

    uint32_t argsCount = runtimeInfo->GetArgsNumber();
    if (argsCount < 2) { // 2: at least have two arguments
        std::string errStr = "Assertion failed: At least have two arguments.";
        auto error = panda::Exception::TypeError(vm, StringRef::NewFromUtf8(vm, errStr.c_str()));
        panda::JSNApi::ThrowException(vm, error);
        return JSValueRef::Undefined(vm);
    }

    Local<JSValueRef> jsArg0 = runtimeInfo->GetCallArgRef(0);
    Local<JSValueRef> jsArg1 = runtimeInfo->GetCallArgRef(1);

    if (!IsEqual(vm, jsArg0, jsArg1)) {
        std::string errStr = std::string("Assertion failed: ").append(jsArg0->ToString(vm)->ToString(vm))
            .append(" != ").append(jsArg1->ToString(vm)->ToString(vm));
        auto error = panda::Exception::TypeError(vm, StringRef::NewFromUtf8(vm, errStr.c_str()));
        panda::JSNApi::ThrowException(vm, error);
    }

    return JSValueRef::Undefined(vm);
}

Local<JSValueRef> AssertTrue(JsiRuntimeCallInfo *runtimeInfo)
{
    EcmaVM *vm = runtimeInfo->GetVM();

    uint32_t argsCount = runtimeInfo->GetArgsNumber();
    if (argsCount < 1) {
        std::string errStr = "Assertion failed: At least have one argument.";
        auto error = panda::Exception::TypeError(vm, StringRef::NewFromUtf8(vm, errStr.c_str()));
        panda::JSNApi::ThrowException(vm, error);
        return JSValueRef::Undefined(vm);
    }

    Local<JSValueRef> jsArg0 = runtimeInfo->GetCallArgRef(0);

    if (!jsArg0->IsTrue()) {
        std::string errStr = std::string("Assertion failed: Expect ").append(jsArg0->ToString(vm)->ToString(vm))
            .append(" equals True.");
        auto error = panda::Exception::TypeError(vm, StringRef::NewFromUtf8(vm, errStr.c_str()));
        panda::JSNApi::ThrowException(vm, error);
    }

    return JSValueRef::Undefined(vm);
}

Local<JSValueRef> AssertUnreachable(JsiRuntimeCallInfo *runtimeInfo)
{
    EcmaVM *vm = runtimeInfo->GetVM();

    LOG_ECMA(FATAL) << "this test is unreachable";
    return JSValueRef::Undefined(vm);
}

bool ExecutePandaFile(EcmaVM *vm, JSRuntimeOptions &runtimeOptions, std::string &files)
{
    bool ret = true;
    LocalScope scope(vm);
    std::string entry = runtimeOptions.GetEntryPoint();
#if defined(PANDA_TARGET_WINDOWS)
    arg_list_t fileNames = base::StringHelper::SplitString(files, ";");
#else
    arg_list_t fileNames = base::StringHelper::SplitString(files, ":");
#endif
    if (runtimeOptions.GetTestAssert()) {
        Local<ObjectRef> globalObj = JSNApi::GetGlobalObject(vm);
        Local<FunctionRef> assertEqual = FunctionRef::New(vm, AssertEqual);
        globalObj->Set(vm, StringRef::NewFromUtf8(vm, "assert_equal"), assertEqual);
        Local<FunctionRef> assertTrue = FunctionRef::New(vm, AssertTrue);
        globalObj->Set(vm, StringRef::NewFromUtf8(vm, "assert_true"), assertTrue);
        Local<FunctionRef> assertUnreachable = FunctionRef::New(vm, AssertUnreachable);
        globalObj->Set(vm, StringRef::NewFromUtf8(vm, "assert_unreachable"), assertUnreachable);
        Local<FunctionRef> testEnd = FunctionRef::New(vm, TestEnd);
        globalObj->Set(vm, StringRef::NewFromUtf8(vm, "test_end"), testEnd);
    }
    if (runtimeOptions.WasAOTOutputFileSet()) {
        JSNApi::LoadAotFile(vm, "");
    }
    ClockScope execute;
    for (const auto &fileName : fileNames) {
        auto res = JSNApi::Execute(vm, fileName, entry);
        if (!res) {
            std::cerr << "Cannot execute panda file '" << fileName << "' with entry '" << entry << "'" << std::endl;
            ret = false;
            break;
        }
    }
    if (runtimeOptions.GetTestAssert() && !g_testEnd) {
        LOG_ECMA(FATAL) << "this test didn't run to the end normally.";
    }
    auto totalTime = execute.TotalSpentTime();

    if (runtimeOptions.IsEnablePrintExecuteTime()) {
        std::cout << "execute pandafile spent time " << totalTime << "ms" << std::endl;
    }
    return ret;
}

int Main(const int argc, const char **argv)
{
    InitializeMallocConfig();
    auto startTime =
        std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::system_clock::now().time_since_epoch())
            .count();

    BlockSignals();

    if (argc < 2) { // 2: at least have two arguments
        std::cerr << GetHelper();
        return -1;
    }

    int newArgc = argc;
    std::string files = argv[argc - 1];
    if (!base::StringHelper::EndsWith(files, ".abc")) {
        std::cerr << "The last argument must be abc file" << std::endl;
        std::cerr << GetHelper();
        return 1;
    }

    newArgc--;
    JSRuntimeOptions runtimeOptions;
    bool retOpt = runtimeOptions.ParseCommand(newArgc, argv);
    if (!runtimeOptions.IsMegaICInitialized()) {
        runtimeOptions.SetEnableMegaIC(true);
    }
    if (!retOpt) {
        std::cerr << GetHelper();
        return 1;
    }

    if (runtimeOptions.IsStartupTime()) {
        std::cout << "\n"
                  << "Startup start time: " << startTime << std::endl;
    }
    bool ret = true;
    EcmaVM *vm = JSNApi::CreateEcmaVM(runtimeOptions);
    if (vm == nullptr) {
        std::cerr << "Cannot Create vm" << std::endl;
        return -1;
    }

    bool isMergeAbc = runtimeOptions.GetMergeAbc();
    JSNApi::SetBundle(vm, !isMergeAbc);
    ret = ExecutePandaFile(vm, runtimeOptions, files);

    JSNApi::DestroyJSVM(vm);
    return ret ? 0 : -1;
}
}  // namespace panda::ecmascript

int main(int argc, const char **argv)
{
    return panda::ecmascript::Main(argc, argv);
}
