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

#include "ecmascript/builtins/builtins_cjs_module.h"

#include "ecmascript/interpreter/interpreter-inl.h"
#include "ecmascript/module/module_path_helper.h"

namespace panda::ecmascript::builtins {

JSTaggedValue BuiltinsCjsModule::CjsModuleConstructor(EcmaRuntimeCallInfo *argv)
{
    JSThread *thread = argv->GetThread();
    BUILTINS_API_TRACE(thread, CjsModule, CjsModuleConstructor);
    [[maybe_unused]] EcmaHandleScope scope(thread);

    LOG_ECMA(ERROR) << "BuiltinsCjsModule::CjsModuleConstructor : can not be call";
    return JSTaggedValue::Undefined();
}

JSTaggedValue BuiltinsCjsModule::Compiler(EcmaRuntimeCallInfo *msg)
{
    ASSERT(msg);
    JSThread *thread = msg->GetThread();
    BUILTINS_API_TRACE(thread, CjsModule, Compiler);
    [[maybe_unused]] EcmaHandleScope handleScope(thread);
    return JSTaggedValue::Hole();
}

JSTaggedValue BuiltinsCjsModule::Load(EcmaRuntimeCallInfo *msg)
{
    ASSERT(msg);
    JSThread *thread = msg->GetThread();
    BUILTINS_API_TRACE(thread, CjsModule, Load);
    [[maybe_unused]] EcmaHandleScope handleScope(thread);
    return JSTaggedValue::Hole();
}

JSTaggedValue BuiltinsCjsModule::ResolveFilename(EcmaRuntimeCallInfo *argv)
{
    ASSERT(argv);
    JSThread *thread = argv->GetThread();
    BUILTINS_API_TRACE(thread, CjsModule, ResolveFilename);
    [[maybe_unused]] EcmaHandleScope handleScope(thread);

    uint32_t length = argv->GetArgsNumber();
    CString parent;
    CString dirname;
    const JSPandaFile *jsPandaFile = EcmaInterpreter::GetNativeCallPandafile(thread);
    ModulePathHelper::ResolveCurrentPath(parent, dirname, jsPandaFile);

    if (length != 1) {  // strange arg's number
        LOG_ECMA(FATAL) << "BuiltinsCjsModule::Load : can only accept one argument";
        UNREACHABLE();
    }
    JSHandle<EcmaString> requestName = JSHandle<EcmaString>::Cast(GetCallArg(argv, 0));
    CString requestNameStr = ModulePathHelper::Utf8ConvertToString(thread, requestName.GetTaggedValue());
    CString filename = ResolveFilenameFromNative(thread, dirname, requestNameStr);
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<JSTaggedValue> filenameHdl = JSHandle<JSTaggedValue>::Cast(factory->NewFromUtf8(filename));
    return filenameHdl.GetTaggedValue();
}

JSTaggedValue BuiltinsCjsModule::Require(EcmaRuntimeCallInfo *msg)
{
    ASSERT(msg);
    JSThread *thread = msg->GetThread();
    BUILTINS_API_TRACE(thread, CjsModule, Require);
    [[maybe_unused]] EcmaHandleScope handleScope(thread);
    return JSTaggedValue::Hole();
}

JSTaggedValue BuiltinsCjsModule::GetExportsForCircularRequire(EcmaRuntimeCallInfo *msg)
{
    ASSERT(msg);
    JSThread *thread = msg->GetThread();
    BUILTINS_API_TRACE(thread, CjsModule, GetExportsForCircularRequire);
    [[maybe_unused]] EcmaHandleScope handleScope(thread);
    return JSTaggedValue::Hole();
}

JSTaggedValue BuiltinsCjsModule::UpdateChildren(EcmaRuntimeCallInfo *msg)
{
    ASSERT(msg);
    JSThread *thread = msg->GetThread();
    BUILTINS_API_TRACE(thread, CjsModule, UpdateChildren);
    [[maybe_unused]] EcmaHandleScope handleScope(thread);
    return JSTaggedValue::Hole();
}
}  // namespace panda::ecmascript::builtins

