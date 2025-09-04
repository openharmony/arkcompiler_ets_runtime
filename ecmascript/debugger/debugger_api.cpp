/*
 * Copyright (c) 2021-2022 Huawei Device Co., Ltd.
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

#include "ecmascript/debugger/debugger_api.h"

#include "ecmascript/base/config.h"
#include "ecmascript/debugger/js_debugger.h"
#include "ecmascript/dfx/stackinfo/async_stack_trace.h"
#include "ecmascript/interpreter/slow_runtime_stub.h"
#include "ecmascript/interpreter/fast_runtime_stub-inl.h"
#include "ecmascript/tagged_hash_array.h"
#include "ecmascript/tagged_tree.h"
#include "ecmascript/js_api/js_api_hashset.h"
#include "ecmascript/js_api/js_api_tree_map.h"
#include "ecmascript/js_api/js_api_tree_set.h"
#include "ecmascript/js_api/js_api_lightweightmap.h"
#include "ecmascript/js_api/js_api_lightweightset.h"
#include "ecmascript/jobs/micro_job_queue.h"
#include "ecmascript/module/module_resolver.h"
#include "ecmascript/module/module_value_accessor.h"

namespace panda::ecmascript::tooling {
using panda::ecmascript::base::ALLOW_BINARY;
using panda::ecmascript::base::ALLOW_HEX;
using panda::ecmascript::base::ALLOW_OCTAL;
using panda::ecmascript::base::NumberHelper;
using ecmascript::JSAPIArrayList;
using ecmascript::JSAPIDeque;
using ecmascript::JSAPIHashMap;
using ecmascript::JSAPIHashSet;
using ecmascript::JSAPILightWeightMap;
using ecmascript::JSAPILightWeightSet;
using ecmascript::JSAPIList;
using ecmascript::JSAPILinkedList;
using ecmascript::JSAPIPlainArray;
using ecmascript::JSAPIStack;
using ecmascript::JSAPIQueue;
using ecmascript::JSAPITreeSet;
using ecmascript::JSAPITreeMap;
using ecmascript::JSAPIVector;
using ecmascript::LinkedNode;
using ecmascript::TaggedHashArray;
using ecmascript::TaggedNode;
using ecmascript::RBTreeNode;
using ecmascript::TaggedTreeSet;
using ecmascript::TaggedTreeMap;
using ecmascript::TaggedQueue;

// FrameHandler
uint32_t DebuggerApi::GetStackDepth(const EcmaVM *ecmaVm)
{
    uint32_t count = 0;
    FrameHandler frameHandler(ecmaVm->GetJSThread());
    for (; frameHandler.HasFrame(); frameHandler.PrevJSFrame()) {
        if (frameHandler.IsEntryFrame() || frameHandler.IsBuiltinFrame()) {
            continue;
        }
        ++count;
    }
    return count;
}

std::shared_ptr<FrameHandler> DebuggerApi::NewFrameHandler(const EcmaVM *ecmaVm)
{
    return std::make_shared<FrameHandler>(ecmaVm->GetJSThread());
}

bool DebuggerApi::StackWalker(const EcmaVM *ecmaVm, std::function<StackState(const FrameHandler *)> func)
{
    FrameHandler frameHandler(ecmaVm->GetJSThread());
    for (; frameHandler.HasFrame(); frameHandler.PrevJSFrame()) {
        if (frameHandler.IsEntryFrame() || frameHandler.IsBuiltinFrame()) {
            continue;
        }
        StackState state = func(&frameHandler);
        if (state == StackState::CONTINUE) {
            continue;
        }
        if (state == StackState::FAILED) {
            return false;
        }
        return true;
    }
    return true;
}

uint32_t DebuggerApi::GetStackDepthOverBuiltin(const EcmaVM *ecmaVm)
{
    uint32_t count = 0;
    FrameHandler frameHandler(ecmaVm->GetJSThread());
    for (; frameHandler.HasFrame(); frameHandler.PrevJSFrame()) {
        if (frameHandler.IsEntryFrame()) {
            if (frameHandler.IsInterpreterBuiltinFrame()) {
                break;
            }
            continue;
        }
        if (frameHandler.IsBuiltinFrame()) {
            break;
        }
        ++count;
    }
    return count;
}

uint32_t DebuggerApi::GetBytecodeOffset(const EcmaVM *ecmaVm)
{
    return FrameHandler(ecmaVm->GetJSThread()).GetBytecodeOffset();
}

std::unique_ptr<PtMethod> DebuggerApi::GetMethod(const EcmaVM *ecmaVm)
{
    JSThread *thread = ecmaVm->GetJSThread();
    FrameHandler frameHandler(thread);
    Method* method = frameHandler.GetMethod();
    std::unique_ptr<PtMethod> ptMethod = std::make_unique<PtMethod>(
        method->GetJSPandaFile(thread), method->GetMethodId(), method->IsNativeWithCallField());
    return ptMethod;
}

void DebuggerApi::SetVRegValue(FrameHandler *frameHandler, size_t index, Local<JSValueRef> value)
{
    return frameHandler->SetVRegValue(index, JSNApiHelper::ToJSTaggedValue(*value));
}

uint32_t DebuggerApi::GetBytecodeOffset(const FrameHandler *frameHandler)
{
    return frameHandler->GetBytecodeOffset();
}

Method *DebuggerApi::GetMethod(const FrameHandler *frameHandler)
{
    return frameHandler->GetMethod();
}

bool DebuggerApi::IsNativeMethod(const EcmaVM *ecmaVm)
{
    FrameHandler frameHandler(ecmaVm->GetJSThread());
    return DebuggerApi::IsNativeMethod(&frameHandler);
}

bool DebuggerApi::IsNativeMethod(const FrameHandler *frameHandler)
{
    if (!frameHandler->HasFrame()) {
        return false;
    }
    Method* method = frameHandler->GetMethod();
    return method->IsNativeWithCallField();
}

JSPandaFile *DebuggerApi::GetJSPandaFile(const EcmaVM *ecmaVm)
{
    JSThread *thread = ecmaVm->GetJSThread();
    Method *method = FrameHandler(thread).GetMethod();
    return const_cast<JSPandaFile *>(method->GetJSPandaFile(thread));
}

JSTaggedValue DebuggerApi::GetEnv(const FrameHandler *frameHandler)
{
    return frameHandler->GetEnv();
}

JSTaggedType *DebuggerApi::GetSp(const FrameHandler *frameHandler)
{
    return frameHandler->GetSp();
}

int32_t DebuggerApi::GetVregIndex(const FrameHandler *frameHandler, std::string_view name)
{
    Method *method = DebuggerApi::GetMethod(frameHandler);
    if (method->IsNativeWithCallField()) {
        LOG_DEBUGGER(ERROR) << "GetVregIndex: native frame not support";
        return -1;
    }
    DebugInfoExtractor *extractor =
        JSPandaFileManager::GetInstance()->GetJSPtExtractor(method->GetJSPandaFile(frameHandler->GetJSThread()));
    if (extractor == nullptr) {
        LOG_DEBUGGER(ERROR) << "GetVregIndex: extractor is null";
        return -1;
    }

    uint32_t currentOffset = frameHandler->GetBytecodeOffset();
    int32_t regNumber = -1;
    uint32_t startOffset = 0;
    uint32_t endOffset = UINT32_MAX;
    auto table = extractor->GetLocalVariableTable(method->GetMethodId());
    for (auto iter = table.begin(); iter != table.end(); iter++) {
        // if currentOffset not in variable's scope, skip it
        if (iter->name == name.data() && currentOffset >= iter->startOffset && currentOffset < iter->endOffset) {
            // if there are multiple variables with the same name, get regNumber with the smallest scope
            if (iter->startOffset >= startOffset && iter->endOffset <= endOffset) {
                regNumber = iter->regNumber;
                startOffset = iter->startOffset;
                endOffset = iter->endOffset;
            }
        }
    }
    if (regNumber != -1) {
        return regNumber;
    }
    return -1;
}

Local<JSValueRef> DebuggerApi::GetVRegValue(const EcmaVM *ecmaVm,
    const FrameHandler *frameHandler, size_t index)
{
    auto value = frameHandler->GetVRegValue(index);
    JSHandle<JSTaggedValue> handledValue(ecmaVm->GetJSThread(), value);
    return JSNApiHelper::ToLocal<JSValueRef>(handledValue);
}

// JSThread
Local<JSValueRef> DebuggerApi::GetAndClearException(const EcmaVM *ecmaVm)
{
    auto exception = ecmaVm->GetJSThread()->GetException();
    JSHandle<JSTaggedValue> handledException(ecmaVm->GetJSThread(), exception);
    ecmaVm->GetJSThread()->ClearException();
    return JSNApiHelper::ToLocal<JSValueRef>(handledException);
}

void DebuggerApi::SetException(const EcmaVM *ecmaVm, Local<JSValueRef> exception)
{
    ecmaVm->GetJSThread()->SetException(JSNApiHelper::ToJSTaggedValue(*exception));
}

void DebuggerApi::ClearException(const EcmaVM *ecmaVm)
{
    return ecmaVm->GetJSThread()->ClearException();
}

// NumberHelper
double DebuggerApi::StringToDouble(const uint8_t *start, const uint8_t *end, uint8_t radix)
{
    return NumberHelper::StringToDouble(start, end, radix, ALLOW_BINARY | ALLOW_HEX | ALLOW_OCTAL);
}

// JSDebugger
JSDebugger *DebuggerApi::CreateJSDebugger(const EcmaVM *ecmaVm)
{
    return new JSDebugger(ecmaVm);
}

void DebuggerApi::DestroyJSDebugger(JSDebugger *debugger)
{
    delete debugger;
}

void DebuggerApi::RegisterHooks(JSDebugger *debugger, PtHooks *hooks)
{
    debugger->RegisterHooks(hooks);
}

bool DebuggerApi::SetBreakpoint(JSDebugger *debugger, const JSPtLocation &location,
    Local<FunctionRef> condFuncRef, bool isSmartBreakpoint)
{
    if (isSmartBreakpoint) {
        return debugger->SetSmartBreakpoint(location);
    }
    return debugger->SetBreakpoint(location, condFuncRef);
}

bool DebuggerApi::RemoveBreakpoint(JSDebugger *debugger, const JSPtLocation &location)
{
    return debugger->RemoveBreakpoint(location);
}

void DebuggerApi::RemoveAllBreakpoints(JSDebugger *debugger)
{
    return debugger->RemoveAllBreakpoints();
}

void DebuggerApi::SetSingleStepStatus(JSDebugger *debugger, bool status)
{
    return debugger->SetSingleStepStatus(status);
}

bool DebuggerApi::GetSingleStepStatus(JSDebugger *debugger)
{
    return debugger->GetSingleStepStatus();
}

int32_t DebuggerApi::GetObjectHash(const EcmaVM *ecmaVM, const JSHandle<JSTaggedValue> &tagged)
{
    if (!tagged->IsECMAObject()) {
        return 0;
    }
    JSThread *thread = ecmaVM->GetJSThread();
    bool hasHash = ECMAObject::Cast(tagged->GetTaggedObject())->HasHash(thread);
    if (!hasHash) {
        int32_t hash = base::RandomGenerator::GenerateIdentityHash();
        auto ecmaObj = ECMAObject::Cast(tagged->GetTaggedObject());
        JSHandle<ECMAObject> ecmaObjHandle(thread, ecmaObj);
        ECMAObject::SetHash(thread, hash, ecmaObjHandle);
        return hash;
    } else {
        return ECMAObject::Cast(tagged->GetTaggedObject())->GetHash(thread);
    }
}

int32_t DebuggerApi::GetObjectHashCode(const EcmaVM *ecmaVM, const JSHandle<JSTaggedValue> &tagged)
{
    if (!tagged->IsECMAObject()) {
        return 0;
    }
    JSThread *thread = ecmaVM->GetJSThread();
    int32_t hash = ECMAObject::Cast(tagged->GetTaggedObject())->GetHash(thread);
    if (hash == 0) {
        hash = base::RandomGenerator::GenerateIdentityHash();
        auto ecmaObjHandle = JSHandle<ECMAObject>::Cast(tagged);
        ECMAObject::SetHash(thread, hash, ecmaObjHandle);
    }
    return hash;
}

void DebuggerApi::GetObjectClassName(const EcmaVM *ecmaVM, Local<JSValueRef> &tagged, std::string &className)
{
    if (!tagged->IsObject(ecmaVM)) {
        return;
    }
    Local<JSValueRef> prototype = Local<ObjectRef>(tagged)->GetPrototype(ecmaVM);
    Local<StringRef> key = StringRef::NewFromUtf8(ecmaVM, "constructor");
    PropertyAttribute protoProperty = PropertyAttribute::Default();
    if (!Local<ObjectRef>(prototype)->GetOwnProperty(ecmaVM, key, protoProperty)) {
        LOG_DEBUGGER(DEBUG) << "DebuggerApi::GetObjectClassName get property [constructor] failed";
        return;
    }
    Local<JSValueRef> constructor = protoProperty.GetValue(ecmaVM);
    if (!constructor->IsNull()) {
        Local<StringRef> constructorFuncName = Local<FunctionRef>(constructor)->GetName(ecmaVM);
        className = constructorFuncName->ToString(ecmaVM);
    }
}

bool DebuggerApi::RemoveBreakpointsByUrl(JSDebugger *debugger, const std::string &url)
{
    return debugger->RemoveBreakpointsByUrl(url);
}

void DebuggerApi::DisableFirstTimeFlag(JSDebugger *debugger)
{
    return debugger->DisableFirstTimeFlag();
}

void DebuggerApi::SetSymbolicBreakpoint(JSDebugger *debugger, const std::unordered_set<std::string> &functionNamesSet)
{
    return debugger->SetSymbolicBreakpoint(functionNamesSet);
}

void DebuggerApi::RemoveSymbolicBreakpoint(JSDebugger* debugger, const std::string &functionName)
{
    return debugger->RemoveSymbolicBreakpoint(functionName);
}

// ScopeInfo
Local<JSValueRef> DebuggerApi::GetProperties(const EcmaVM *ecmaVm, const FrameHandler *frameHandler,
                                             int32_t level, uint32_t slot)
{
    JSTaggedValue env = frameHandler->GetEnv();
    JSThread *thread = ecmaVm->GetJSThread();
    for (int i = 0; i < level; i++) {
        JSTaggedValue taggedParentEnv = LexicalEnv::Cast(env.GetTaggedObject())->GetParentEnv(thread);
        ASSERT(taggedParentEnv.IsLexicalEnv());
        env = taggedParentEnv;
    }
    JSTaggedValue value = LexicalEnv::Cast(env.GetTaggedObject())->GetProperties(thread, slot) ;
    JSHandle<JSTaggedValue> handledValue(thread, value);
    return JSNApiHelper::ToLocal<JSValueRef>(handledValue);
}

void DebuggerApi::SetProperties(const EcmaVM *ecmaVm, const FrameHandler *frameHandler,
                                int32_t level, uint32_t slot, Local<JSValueRef> value)
{
    JSTaggedValue env = frameHandler->GetEnv();
    JSThread *thread = ecmaVm->GetJSThread();
    for (int i = 0; i < level; i++) {
        JSTaggedValue taggedParentEnv = LexicalEnv::Cast(env.GetTaggedObject())->GetParentEnv(thread);
        ASSERT(taggedParentEnv.IsLexicalEnv());
        env = taggedParentEnv;
    }
    JSTaggedValue target = JSNApiHelper::ToJSHandle(value).GetTaggedValue();
    LexicalEnv::Cast(env.GetTaggedObject())->SetProperties(thread, slot, target);
}

std::pair<int32_t, uint32_t> DebuggerApi::GetLevelSlot(const JSThread *thread, const FrameHandler *frameHandler,
    std::string_view name)
{
    int32_t level = 0;
    uint32_t slot = 0;
    JSTaggedValue curEnv = frameHandler->GetEnv();
    for (; curEnv.IsLexicalEnv(); curEnv = LexicalEnv::Cast(curEnv.GetTaggedObject())->GetParentEnv(thread), level++) {
        LexicalEnv *lexicalEnv = LexicalEnv::Cast(curEnv.GetTaggedObject());
        if (lexicalEnv->GetScopeInfo(thread).IsHole()) {
            continue;
        }
        auto result =
            JSNativePointer::Cast(lexicalEnv->GetScopeInfo(thread).GetTaggedObject())->GetExternalPointer();
        ScopeDebugInfo *scopeDebugInfo = reinterpret_cast<ScopeDebugInfo *>(result);
        auto iter = scopeDebugInfo->scopeInfo.find(name.data());
        if (iter == scopeDebugInfo->scopeInfo.end()) {
            continue;
        }
        slot = iter->second;
        return std::make_pair(level, slot);
    }
    return std::make_pair(-1, 0);
}

Local<JSValueRef> DebuggerApi::GetGlobalValue(const EcmaVM *ecmaVm, const FrameHandler *frameHandler,
                                              Local<StringRef> name)
{
    JSTaggedValue result;
    Local<JSValueRef> globalEnv = GetCurrentGlobalEnv(ecmaVm, frameHandler);
    JSHandle<JSTaggedValue> globalObj = JSNApiHelper::ToJSHandle(JSNApi::GetGlobalObject(ecmaVm, globalEnv));
    JSThread *thread = ecmaVm->GetJSThread();

    JSTaggedValue key = JSNApiHelper::ToJSTaggedValue(*name);
    JSTaggedValue globalRec = SlowRuntimeStub::LdGlobalRecord(thread, key);
    if (!globalRec.IsUndefined()) {
        ASSERT(globalRec.IsPropertyBox());
        result = PropertyBox::Cast(globalRec.GetTaggedObject())->GetValue(thread);
        return JSNApiHelper::ToLocal<JSValueRef>(JSHandle<JSTaggedValue>(thread, result));
    }

    JSTaggedValue globalVar = FastRuntimeStub::GetGlobalOwnProperty(thread, globalObj.GetTaggedValue(), key);
    if (!globalVar.IsHole()) {
        return JSNApiHelper::ToLocal<JSValueRef>(JSHandle<JSTaggedValue>(thread, globalVar));
    } else {
        result = SlowRuntimeStub::TryLdGlobalByNameFromGlobalProto(thread, globalObj.GetTaggedValue(), key);
        return JSNApiHelper::ToLocal<JSValueRef>(JSHandle<JSTaggedValue>(thread, result));
    }

    return Local<JSValueRef>();
}

bool DebuggerApi::SetGlobalValue(const EcmaVM *ecmaVm, const FrameHandler *frameHandler,
                                 Local<StringRef> name, Local<JSValueRef> value)
{
    JSTaggedValue result;
    JSTaggedValue globalObj = ecmaVm->GetGlobalEnv()->GetGlobalObject();
    JSThread *thread = ecmaVm->GetJSThread();

    JSTaggedValue key = JSNApiHelper::ToJSTaggedValue(*name);
    JSTaggedValue newVal = JSNApiHelper::ToJSTaggedValue(*value);
    JSTaggedValue globalRec = SlowRuntimeStub::LdGlobalRecord(thread, key);
    if (!globalRec.IsUndefined()) {
        result = SlowRuntimeStub::TryUpdateGlobalRecord(thread, key, newVal);
        return !result.IsException();
    }

    JSTaggedValue globalVar = FastRuntimeStub::GetGlobalOwnProperty(thread, globalObj, key);
    if (!globalVar.IsHole()) {
        result = SlowRuntimeStub::StGlobalVar(thread, key, newVal);
        return !result.IsException();
    }

    return false;
}

JSTaggedValue DebuggerApi::GetCurrentModule(const EcmaVM *ecmaVm)
{
    JSThread *thread = ecmaVm->GetJSThread();
    FrameHandler frameHandler(thread);
    for (; frameHandler.HasFrame(); frameHandler.PrevJSFrame()) {
        if (frameHandler.IsEntryFrame()) {
            continue;
        }
        Method *method = frameHandler.GetMethod();
        // Skip builtins method
        if (method->IsNativeWithCallField()) {
            continue;
        }
        JSTaggedValue func = frameHandler.GetFunction();
        JSTaggedValue module = JSFunction::Cast(func.GetTaggedObject())->GetModule(thread);
        if (module.IsUndefined()) {
            continue;
        }
        return module;
    }
    UNREACHABLE();
}

JSHandle<JSTaggedValue> DebuggerApi::GetImportModule(const EcmaVM *ecmaVm,
                                                     const JSHandle<JSTaggedValue> &currentModule, std::string &name)
{
    JSThread *thread = ecmaVm->GetJSThread();
    JSMutableHandle<JSTaggedValue> importModule(thread, thread->GlobalConstants()->GetUndefined());
    if (!currentModule->IsSourceTextModule()) {
        return importModule;
    }

    JSTaggedValue importEntries = SourceTextModule::Cast(currentModule->GetTaggedObject())->GetImportEntries(thread);
    if (importEntries.IsUndefined()) {
        return importModule;
    }

    JSHandle<TaggedArray> importArray(thread, TaggedArray::Cast(importEntries.GetTaggedObject()));
    size_t importEntriesLen = importArray->GetLength();
    JSHandle<JSTaggedValue> starString = thread->GlobalConstants()->GetHandledStarString();
    JSMutableHandle<ImportEntry> ee(thread, thread->GlobalConstants()->GetUndefined());
    JSMutableHandle<TaggedArray> environment(thread, thread->GlobalConstants()->GetUndefined());
    for (size_t idx = 0; idx < importEntriesLen; idx++) {
        ee.Update(importArray->Get(thread, idx));
        JSTaggedValue localName = ee->GetLocalName(thread);
        JSTaggedValue importName = ee->GetImportName(thread);
        // Skip 'import * as name from xxx'
        if (localName.IsString() && !JSTaggedValue::SameValue(thread, importName, starString.GetTaggedValue())) {
            std::string varName = EcmaStringAccessor(localName).ToStdString(thread);
            if (varName != name) {
                continue;
            }
            JSTaggedValue moduleEnvironment = SourceTextModule::Cast(
                currentModule->GetTaggedObject())->GetEnvironment(thread);
            environment.Update(moduleEnvironment);
            JSTaggedValue resolvedBinding = environment->Get(thread, idx);
            ResolvedIndexBinding *binding = ResolvedIndexBinding::Cast(resolvedBinding.GetTaggedObject());
            importModule.Update(binding->GetModule(thread));
            name = EcmaStringAccessor(importName).ToStdString(thread);
            return importModule;
        }
    }
    return importModule;
}

int32_t DebuggerApi::GetModuleVariableIndex(const EcmaVM *ecmaVm, const JSHandle<JSTaggedValue> &currentModule,
                                            std::string &name)
{
    if (!currentModule->IsSourceTextModule()) {
        return -1;
    }
    JSThread *thread = ecmaVm->GetJSThread();
    JSTaggedValue dictionary = SourceTextModule::Cast(currentModule->GetTaggedObject())->GetNameDictionary(thread);
    if (dictionary.IsUndefined()) {
        return -1;
    }

    if (dictionary.IsTaggedArray()) {
        JSTaggedValue localExportEntries = SourceTextModule::Cast(
            currentModule->GetTaggedObject())->GetLocalExportEntries(thread);
        ASSERT(localExportEntries.IsTaggedArray());
        JSHandle<TaggedArray> localExportArray(thread, TaggedArray::Cast(localExportEntries.GetTaggedObject()));
        uint32_t exportEntriesLen = localExportArray->GetLength();
        JSMutableHandle<LocalExportEntry> ee(thread, thread->GlobalConstants()->GetUndefined());
        for (uint32_t idx = 0; idx < exportEntriesLen; idx++) {
            ee.Update(localExportArray->Get(thread, idx));
            JSTaggedValue localKey = ee->GetLocalName(thread);
            JSTaggedValue exportKey = ee->GetExportName(thread);
            if (localKey.IsString() && exportKey.IsString()) {
                std::string localName = EcmaStringAccessor(localKey).ToStdString(thread);
                std::string exportName = EcmaStringAccessor(exportKey).ToStdString(thread);
                if (localName == name || exportName == name) {
                    return idx;
                }
            }
        }
    }
    return -1;
}

Local<JSValueRef> DebuggerApi::GetExportVariableValue(const EcmaVM *ecmaVm,
                                                      const JSHandle<JSTaggedValue> &currentModule, std::string &name)
{
    Local<JSValueRef> result;
    if (!currentModule->IsSourceTextModule()) {
        return result;
    }
    int32_t index = GetModuleVariableIndex(ecmaVm, currentModule, name);
    if (index == -1) {
        return result;
    }

    JSThread *thread = ecmaVm->GetJSThread();
    JSTaggedValue dictionary = SourceTextModule::Cast(currentModule->GetTaggedObject())->GetNameDictionary(thread);
    if (dictionary.IsUndefined()) {
        return result;
    }

    if (dictionary.IsTaggedArray()) {
        TaggedArray *array = TaggedArray::Cast(dictionary.GetTaggedObject());
        JSTaggedValue moduleValue = array->Get(thread, index);
        result = JSNApiHelper::ToLocal<JSValueRef>(JSHandle<JSTaggedValue>(thread, moduleValue));
        return result;
    }
    return result;
}

bool DebuggerApi::SetExportVariableValue(const EcmaVM *ecmaVm, const JSHandle<JSTaggedValue> &currentModule,
                                         std::string &name, Local<JSValueRef> value)
{
    if (!currentModule->IsSourceTextModule()) {
        return false;
    }
    int32_t index = GetModuleVariableIndex(ecmaVm, currentModule, name);
    if (index == -1) {
        return false;
    }

    JSThread *thread = ecmaVm->GetJSThread();
    JSTaggedValue dictionary = SourceTextModule::Cast(currentModule->GetTaggedObject())->GetNameDictionary(thread);
    if (dictionary.IsUndefined()) {
        return false;
    }

    JSTaggedValue curValue = JSNApiHelper::ToJSTaggedValue(*value);
    if (dictionary.IsTaggedArray()) {
        TaggedArray *array = TaggedArray::Cast(dictionary.GetTaggedObject());
        array->Set(thread, index, curValue);
        return true;
    }
    return false;
}

Local<JSValueRef> DebuggerApi::GetModuleValue(const EcmaVM *ecmaVm, const JSHandle<JSTaggedValue> &currentModule,
                                              std::string &name)
{
    Local<JSValueRef> result;
    if (!currentModule->IsSourceTextModule()) {
        return result;
    }
    // Get variable from local export
    result = GetExportVariableValue(ecmaVm, currentModule, name);
    if (!result.IsEmpty()) {
        return result;
    }
    // Get variable from import module
    JSHandle<JSTaggedValue> importModule = GetImportModule(ecmaVm, currentModule, name);
    result = GetExportVariableValue(ecmaVm, importModule, name);
    return result;
}

bool DebuggerApi::SetModuleValue(const EcmaVM *ecmaVm, const JSHandle<JSTaggedValue> &currentModule,
                                 std::string &name, Local<JSValueRef> value)
{
    bool result;
    if (!currentModule->IsSourceTextModule()) {
        return false;
    }
    // Set local export variable
    result = SetExportVariableValue(ecmaVm, currentModule, name, value);
    if (result == true) {
        return result;
    }
    // Set import module variable
    JSHandle<JSTaggedValue> importModule = GetImportModule(ecmaVm, currentModule, name);
    result = SetExportVariableValue(ecmaVm, importModule, name, value);
    if (result == true) {
        return result;
    }
    return false;
}

void DebuggerApi::InitializeExportVariables(const EcmaVM *ecmaVm, Local<ObjectRef> &moduleObj,
                                            const JSHandle<JSTaggedValue> &currentModule)
{
    if (!currentModule->IsSourceTextModule()) {
        return;
    }
    JSThread *thread = ecmaVm->GetJSThread();
    JSTaggedValue localExportEntries = SourceTextModule::Cast(
        currentModule->GetTaggedObject())->GetLocalExportEntries(thread);
    if (localExportEntries.IsUndefined()) {
        return;
    }

    JSHandle<TaggedArray> localExportArray(thread, TaggedArray::Cast(localExportEntries.GetTaggedObject()));
    uint32_t exportEntriesLen = localExportArray->GetLength();
    JSMutableHandle<LocalExportEntry> ee(thread, thread->GlobalConstants()->GetUndefined());
    JSMutableHandle<JSTaggedValue> name(thread, thread->GlobalConstants()->GetUndefined());
    JSMutableHandle<JSTaggedValue> value(thread, thread->GlobalConstants()->GetUndefined());
    JSTaggedValue moduleValue = JSTaggedValue::Undefined();
    for (uint32_t idx = 0; idx < exportEntriesLen; idx++) {
        ee.Update(localExportArray->Get(thread, idx));
        JSTaggedValue key = ee->GetLocalName(thread);
        name.Update(key);
        value.Update(moduleValue);
        if (key.IsString()) {
            Local<JSValueRef> variableName = JSNApiHelper::ToLocal<JSValueRef>(name);
            Local<JSValueRef> variableValue = JSNApiHelper::ToLocal<JSValueRef>(value);
            PropertyAttribute descriptor(variableValue, true, true, true);
            moduleObj->DefineProperty(ecmaVm, variableName, descriptor);
        }
    }
}

void DebuggerApi::GetLocalExportVariables(const EcmaVM *ecmaVm, Local<ObjectRef> &moduleObj,
                                          const JSHandle<JSTaggedValue> &currentModule, bool isImportStar)
{
    if (!currentModule->IsSourceTextModule()) {
        return;
    }
    JSThread *thread = ecmaVm->GetJSThread();
    JSTaggedValue dictionary = SourceTextModule::Cast(currentModule->GetTaggedObject())->GetNameDictionary(thread);
    if (dictionary.IsUndefined()) {
        InitializeExportVariables(ecmaVm, moduleObj, currentModule);
        return;
    }

    JSMutableHandle<JSTaggedValue> name(thread, thread->GlobalConstants()->GetUndefined());
    JSMutableHandle<JSTaggedValue> value(thread, thread->GlobalConstants()->GetUndefined());
    if (dictionary.IsTaggedArray()) {
        JSTaggedValue localExportEntries = SourceTextModule::Cast(
            currentModule->GetTaggedObject())->GetLocalExportEntries(thread);
        ASSERT(localExportEntries.IsTaggedArray());
        JSHandle<TaggedArray> localExportArray(thread, TaggedArray::Cast(localExportEntries.GetTaggedObject()));
        uint32_t exportEntriesLen = localExportArray->GetLength();
        JSHandle<TaggedArray> dict(thread, TaggedArray::Cast(dictionary.GetTaggedObject()));
        uint32_t valueLen = dict->GetLength();
        if (exportEntriesLen != valueLen) {
            LOG_FULL(FATAL) << "Key does not match value";
        }

        JSMutableHandle<LocalExportEntry> ee(thread, thread->GlobalConstants()->GetUndefined());
        for (uint32_t idx = 0; idx < exportEntriesLen; idx++) {
            ee.Update(localExportArray->Get(thread, idx));
            JSTaggedValue key;
            if (isImportStar) {
                key = ee->GetExportName(thread);
            } else {
                key = ee->GetLocalName(thread);
            }
            name.Update(key);
            JSTaggedValue moduleValue = dict->Get(thread, ee->GetLocalIndex());
            if (moduleValue.IsHole()) {
                moduleValue = JSTaggedValue::Undefined();
            }
            value.Update(moduleValue);
            if (key.IsString()) {
                Local<JSValueRef> variableName = JSNApiHelper::ToLocal<JSValueRef>(name);
                Local<JSValueRef> variableValue = JSNApiHelper::ToLocal<JSValueRef>(value);
                PropertyAttribute descriptor(variableValue, true, true, true);
                moduleObj->DefineProperty(ecmaVm, variableName, descriptor);
            }
        }
    }
}

void DebuggerApi::GetIndirectExportVariables(const EcmaVM *ecmaVm, Local<ObjectRef> &moduleObj,
                                             const JSHandle<JSTaggedValue> &currentModule)
{
    if (!currentModule->IsSourceTextModule()) {
        return;
    }
    JSHandle<SourceTextModule> module = JSHandle<SourceTextModule>::Cast(currentModule);
    JSThread *thread = ecmaVm->GetJSThread();
    JSTaggedValue indirectExportEntries = module->GetIndirectExportEntries(thread);
    if (indirectExportEntries.IsUndefined()) {
        return;
    }
    ASSERT(indirectExportEntries.IsTaggedArray());
    JSHandle<TaggedArray> indirectExportArray(thread, TaggedArray::Cast(indirectExportEntries.GetTaggedObject()));
    JSHandle<TaggedArray> requestedModules(thread, module->GetRequestedModules(thread));
    uint32_t indirectExportEntriesLen = indirectExportArray->GetLength();
    JSMutableHandle<IndirectExportEntry> ee(thread, thread->GlobalConstants()->GetUndefined());
    JSMutableHandle<JSTaggedValue> name(thread, thread->GlobalConstants()->GetUndefined());
    for (uint32_t idx = 0; idx < indirectExportEntriesLen; idx++) {
        ee.Update(indirectExportArray->Get(thread, idx));
        JSTaggedValue key = ee->GetImportName(thread);
        name.Update(key);
        if (key.IsString()) {
            JSHandle<JSTaggedValue> importModule =
                JSHandle<JSTaggedValue>::Cast(SourceTextModule::GetModuleFromCacheOrResolveNewOne(
                    thread, module, requestedModules, ee->GetModuleRequestIndex()));
            RETURN_IF_ABRUPT_COMPLETION(thread);
            ASSERT(importModule->IsSourceTextModule());
            std::string importName = EcmaStringAccessor(ee->GetImportName(thread)).ToStdString(thread);
            Local<JSValueRef> value = GetModuleValue(ecmaVm, importModule, importName);
            PropertyAttribute descriptor(value, true, true, true);
            Local<JSValueRef> variableName = JSNApiHelper::ToLocal<JSValueRef>(name);
            moduleObj->DefineProperty(ecmaVm, variableName, descriptor);
        }
    }
}

void DebuggerApi::GetImportVariables(const EcmaVM *ecmaVm, Local<ObjectRef> &moduleObj,
                                     const JSHandle<JSTaggedValue> &currentModule)
{
    if (!currentModule->IsSourceTextModule()) {
        return;
    }
    JSHandle<SourceTextModule> module = JSHandle<SourceTextModule>::Cast(currentModule);
    JSThread *thread = ecmaVm->GetJSThread();
    JSTaggedValue importEntries = module->GetImportEntries(thread);
    if (importEntries.IsUndefined()) {
        return;
    }

    JSHandle<TaggedArray> importArray(thread, TaggedArray::Cast(importEntries.GetTaggedObject()));
    JSHandle<TaggedArray> requestedModules(thread, module->GetRequestedModules(thread));
    uint32_t importEntriesLen = importArray->GetLength();
    JSHandle<JSTaggedValue> starString = thread->GlobalConstants()->GetHandledStarString();
    JSMutableHandle<ImportEntry> ee(thread, thread->GlobalConstants()->GetUndefined());
    JSMutableHandle<JSTaggedValue> name(thread, thread->GlobalConstants()->GetUndefined());
    for (uint32_t idx = 0; idx < importEntriesLen; idx++) {
        ee.Update(importArray->Get(thread, idx));
        JSTaggedValue key = ee->GetImportName(thread);
        JSTaggedValue localName = ee->GetLocalName(thread);
        name.Update(localName);
        if (JSTaggedValue::SameValue(thread, key, starString.GetTaggedValue())) {
            JSHandle<JSTaggedValue> importModule =
                JSHandle<JSTaggedValue>::Cast(SourceTextModule::GetModuleFromCacheOrResolveNewOne(
                    thread, module, requestedModules, ee->GetModuleRequestIndex()));
            RETURN_IF_ABRUPT_COMPLETION(thread);
            ASSERT(importModule->IsSourceTextModule());
            Local<ObjectRef> importModuleObj = ObjectRef::New(ecmaVm);
            GetLocalExportVariables(ecmaVm, importModuleObj, importModule, true);
            Local<JSValueRef> variableName = JSNApiHelper::ToLocal<JSValueRef>(name);
            PropertyAttribute descriptor(static_cast<Local<JSValueRef>>(importModuleObj), true, true, true);
            moduleObj->DefineProperty(ecmaVm, variableName, descriptor);
            continue;
        }
        JSTaggedValue moduleValue =
            ModuleValueAccessor::GetModuleValueOuter(thread, idx, currentModule);
        Local<JSValueRef> value = JSNApiHelper::ToLocal<JSValueRef>(JSHandle<JSTaggedValue>(thread, moduleValue));
        Local<JSValueRef> variableName = JSNApiHelper::ToLocal<JSValueRef>(name);
        PropertyAttribute descriptor(value, true, true, true);
        moduleObj->DefineProperty(ecmaVm, variableName, descriptor);
    }
}

void DebuggerApi::HandleUncaughtException(const EcmaVM *ecmaVm, std::string &message)
{
    JSThread *thread = ecmaVm->GetJSThread();
    [[maybe_unused]] EcmaHandleScope handleScope(thread);
    const GlobalEnvConstants *globalConst = thread->GlobalConstants();

    JSHandle<JSTaggedValue> exHandle(thread, thread->GetException());
    thread->ClearException();
    if (exHandle->IsJSError()) {
        JSHandle<JSTaggedValue> nameKey = globalConst->GetHandledNameString();
        JSHandle<EcmaString> name(JSObject::GetProperty(thread, exHandle, nameKey).GetValue());
        JSHandle<JSTaggedValue> msgKey = globalConst->GetHandledMessageString();
        JSHandle<EcmaString> msg(JSObject::GetProperty(thread, exHandle, msgKey).GetValue());
        message = ConvertToString(thread, *name) + ": " + ConvertToString(thread, *msg);
    } else {
        JSHandle<EcmaString> ecmaStr = JSTaggedValue::ToString(thread, exHandle);
        message = ConvertToString(thread, *ecmaStr);
    }
}

Local<FunctionRef> DebuggerApi::GenerateFuncFromBuffer(const EcmaVM *ecmaVm, const void *buffer,
                                                       size_t size, std::string_view entryPoint)
{
    JSPandaFileManager *mgr = JSPandaFileManager::GetInstance();
    std::shared_ptr<JSPandaFile> jsPandaFile =
        mgr->LoadJSPandaFile(ecmaVm->GetJSThread(), "", entryPoint, buffer, size);
    if (jsPandaFile == nullptr) {
        return JSValueRef::Undefined(ecmaVm);
    }

    JSHandle<Program> program = mgr->GenerateProgram(const_cast<EcmaVM *>(ecmaVm), jsPandaFile.get(), entryPoint);
    JSTaggedValue func = program->GetMainFunction(ecmaVm->GetJSThread());
    return JSNApiHelper::ToLocal<FunctionRef>(JSHandle<JSTaggedValue>(ecmaVm->GetJSThread(), func));
}

Local<JSValueRef> DebuggerApi::EvaluateViaFuncCall(EcmaVM *ecmaVm, Local<FunctionRef> funcRef,
    std::shared_ptr<FrameHandler> &frameHandler)
{
    JSNApi::EnableUserUncaughtErrorHandler(ecmaVm);

    JsDebuggerManager *mgr = ecmaVm->GetJsDebuggerManager();
    bool prevDebugMode = mgr->IsDebugMode();
    mgr->SetEvalFrameHandler(frameHandler);
    mgr->SetDebugMode(false); // in order to catch exception
    ecmaVm->GetJSThread()->CheckSwitchDebuggerBCStub();
    std::vector<Local<JSValueRef>> args;
    auto result = funcRef->Call(ecmaVm, JSValueRef::Undefined(ecmaVm), args.data(), args.size());
    mgr->SetDebugMode(prevDebugMode);
    ecmaVm->GetJSThread()->CheckSwitchDebuggerBCStub();
    mgr->SetEvalFrameHandler(nullptr);

    return result;
}

Local<JSValueRef> DebuggerApi::CallFunctionOnCall(EcmaVM *ecmaVm, Local<FunctionRef> funcRef,
    std::shared_ptr<FrameHandler> &frameHandler)
{
    JSNApi::EnableUserUncaughtErrorHandler(ecmaVm);

    JsDebuggerManager *mgr = ecmaVm->GetJsDebuggerManager();
    bool prevDebugMode = mgr->IsDebugMode();
    mgr->SetEvalFrameHandler(frameHandler);
    mgr->SetDebugMode(false);
    ecmaVm->GetJSThread()->CheckSwitchDebuggerBCStub();
    std::vector<Local<JSValueRef>> args;
    auto result = funcRef->Call(ecmaVm, JSValueRef::Undefined(ecmaVm), args.data(), args.size());
    mgr->SetDebugMode(prevDebugMode);
    ecmaVm->GetJSThread()->CheckSwitchDebuggerBCStub();
    mgr->SetEvalFrameHandler(nullptr);

    return result;
}

bool DebuggerApi::IsExceptionCaught(const EcmaVM *ecmaVm)
{
    JSThread *thread = ecmaVm->GetJSThread();
    FrameHandler frameHandler(thread);
    for (; frameHandler.HasFrame(); frameHandler.PrevJSFrame()) {
        if (frameHandler.IsEntryFrame()) {
            return false;
        }
        auto method = frameHandler.GetMethod();
        if (method->FindCatchBlock(thread, frameHandler.GetBytecodeOffset()) != INVALID_INDEX) {
            return true;
        }
    }
    return false;
}

std::vector<DebugInfoExtractor *> DebuggerApi::GetPatchExtractors(const EcmaVM *ecmaVm, const std::string &url)
{
    const auto *hotReloadManager = ecmaVm->GetJsDebuggerManager()->GetHotReloadManager();
    return hotReloadManager->GetPatchExtractors(url);
}

const JSPandaFile *DebuggerApi::GetBaseJSPandaFile(const EcmaVM *ecmaVm, const JSPandaFile *jsPandaFile)
{
    const auto *hotReloadManager = ecmaVm->GetJsDebuggerManager()->GetHotReloadManager();
    return hotReloadManager->GetBaseJSPandaFile(jsPandaFile);
}

std::vector<void *> DebuggerApi::GetNativePointer(const EcmaVM *ecmaVm)
{
    void *native = nullptr;
    std::vector<void *> nativePointer;
    JSThread *thread = ecmaVm->GetJSThread();
    JSTaggedType *current = const_cast<JSTaggedType *>(thread->GetCurrentFrame());
    FrameIterator it(current, thread);
    for (; !it.Done(); it.Advance<GCVisitedFlag::HYBRID_STACK>()) {
        if (!it.IsJSFrame()) {
            continue;
        }
        auto method = it.CheckAndGetMethod();
        if (method == nullptr) {
            continue;
        }

        if (method->IsNativeWithCallField()) {
            JSTaggedValue function = it.GetFunction();
            JSHandle<JSTaggedValue> extraInfoValue(
                thread, JSFunctionBase::Cast(function.GetTaggedObject())->GetFunctionExtraInfo(thread));
            auto cb = ecmaVm->GetNativePtrGetter();
            if (extraInfoValue->IsJSNativePointer() && cb != nullptr) {
                JSHandle<JSNativePointer> extraInfo(extraInfoValue);
                native = cb(reinterpret_cast<void *>(extraInfo->GetData()));
                nativePointer.push_back(native);
            }
        } else {
            nativePointer.push_back(nullptr); // to tell IDE this frame don't hava nativePointer
        }
    }
    return nativePointer;
}

uint32_t DebuggerApi::GetContainerLength(const EcmaVM *ecmaVm, Local<JSValueRef> value)
{
    uint32_t length = Local<ArrayRef>(value)->Length(ecmaVm);
    return length;
}

void DebuggerApi::AddInternalProperties(const EcmaVM *ecmaVm, Local<ObjectRef> object,
                                        ArkInternalValueType type, Global<MapRef> internalObjects)
{
    if (internalObjects.IsEmpty()) {
        return;
    }
    internalObjects->Set(ecmaVm, object, NumberRef::New(ecmaVm, static_cast<int32_t>(type)));
}

Local<JSValueRef> DebuggerApi::GetArrayListValue(const EcmaVM *ecmaVm, Local<JSValueRef> value,
                                                 Global<MapRef> internalObjects)
{
    JSHandle<JSAPIArrayList> arrayList(JSNApiHelper::ToJSHandle(value));
    JSThread *thread = ecmaVm->GetJSThread();
    uint32_t size = static_cast<uint32_t>(arrayList->GetSize(thread));
    Local<JSValueRef> jsValueRef = ArrayRef::New(ecmaVm, size);
    JSMutableHandle<JSTaggedValue> currentValue(thread, JSTaggedValue::Undefined());
    uint32_t index = 0;
    while (index < size) {
        currentValue.Update(arrayList->Get(thread, index));
        ArrayRef::SetValueAt(ecmaVm, jsValueRef, index++, JSNApiHelper::ToLocal<JSValueRef>(currentValue));
    }
    AddInternalProperties(ecmaVm, jsValueRef, ArkInternalValueType::Entry, internalObjects);
    return jsValueRef;
}

Local<JSValueRef> DebuggerApi::GetDequeValue(const EcmaVM *ecmaVm, Local<JSValueRef> value,
                                             Global<MapRef> internalObjects)
{
    JSHandle<JSAPIDeque> deque(JSNApiHelper::ToJSHandle(value));
    JSThread *thread = ecmaVm->GetJSThread();
    uint32_t size = static_cast<uint32_t>(deque->GetSize(thread));
    Local<JSValueRef> jsValueRef = ArrayRef::New(ecmaVm, size);
    JSMutableHandle<JSTaggedValue> currentValue(thread, JSTaggedValue::Undefined());
    uint32_t index = 0;
    while (index < size) {
        currentValue.Update(deque->Get(thread, index));
        ArrayRef::SetValueAt(ecmaVm, jsValueRef, index++, JSNApiHelper::ToLocal<JSValueRef>(currentValue));
    }
    AddInternalProperties(ecmaVm, jsValueRef, ArkInternalValueType::Entry, internalObjects);
    return jsValueRef;
}

Local<JSValueRef> DebuggerApi::GetHashMapValue(const EcmaVM *ecmaVm, Local<JSValueRef> value,
                                               Global<MapRef> internalObjects)
{
    JSHandle<JSAPIHashMap> hashMap(JSNApiHelper::ToJSHandle(value));
    JSThread *thread = ecmaVm->GetJSThread();
    JSHandle<TaggedHashArray> table(thread, hashMap->GetTable(thread));
    uint32_t length = table->GetLength();
    uint32_t size = static_cast<uint32_t>(hashMap->GetSize());
    Local<JSValueRef> jsValueRef = ArrayRef::New(ecmaVm, size);
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSMutableHandle<TaggedQueue> queue(thread, factory->NewTaggedQueue(0));
    JSMutableHandle<TaggedNode> node(thread, JSTaggedValue::Undefined());
    JSMutableHandle<JSTaggedValue> currentKey(thread, JSTaggedValue::Undefined());
    JSMutableHandle<JSTaggedValue> currentValue(thread, JSTaggedValue::Undefined());
    Local<JSValueRef> jsKey = StringRef::NewFromUtf8(ecmaVm, "key");
    Local<JSValueRef> jsValue = StringRef::NewFromUtf8(ecmaVm, "value");
    uint32_t pos = 0;
    uint32_t index = 0;
    while (index < length) {
        node.Update(TaggedHashArray::GetCurrentNode(thread, queue, table, index));
        if (!node.GetTaggedValue().IsHole()) {
            currentKey.Update(node->GetKey(thread));
            currentValue.Update(node->GetValue(thread));
            Local<ObjectRef> objRef = ObjectRef::New(ecmaVm);
            objRef->Set(ecmaVm, jsKey, JSNApiHelper::ToLocal<JSValueRef>(currentKey));
            objRef->Set(ecmaVm, jsValue, JSNApiHelper::ToLocal<JSValueRef>(currentValue));
            AddInternalProperties(ecmaVm, objRef, ArkInternalValueType::Entry, internalObjects);
            ArrayRef::SetValueAt(ecmaVm, jsValueRef, pos++, objRef);
        }
    }
    AddInternalProperties(ecmaVm, jsValueRef, ArkInternalValueType::Entry, internalObjects);
    return jsValueRef;
}

Local<JSValueRef> DebuggerApi::GetHashSetValue(const EcmaVM *ecmaVm, Local<JSValueRef> value,
                                               Global<MapRef> internalObjects)
{
    JSHandle<JSAPIHashSet> hashSet(JSNApiHelper::ToJSHandle(value));
    JSThread *thread = ecmaVm->GetJSThread();
    JSHandle<TaggedHashArray> table(thread, hashSet->GetTable(thread));
    uint32_t length = table->GetLength();
    uint32_t size = static_cast<uint32_t>(hashSet->GetSize());
    Local<JSValueRef> jsValueRef = ArrayRef::New(ecmaVm, size);
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSMutableHandle<TaggedQueue> queue(thread, factory->NewTaggedQueue(0));
    JSMutableHandle<TaggedNode> node(thread, JSTaggedValue::Undefined());
    JSMutableHandle<JSTaggedValue> currentKey(thread, JSTaggedValue::Undefined());
    Local<JSValueRef> jsValue = StringRef::NewFromUtf8(ecmaVm, "value");
    uint32_t pos = 0;
    uint32_t index = 0;
    while (index < length) {
        node.Update(TaggedHashArray::GetCurrentNode(thread, queue, table, index));
        if (!node.GetTaggedValue().IsHole()) {
            currentKey.Update(node->GetKey(thread));
            if (currentKey->IsECMAObject()) {
                Local<ObjectRef> objRef = ObjectRef::New(ecmaVm);
                objRef->Set(ecmaVm, jsValue, JSNApiHelper::ToLocal<JSValueRef>(currentKey));
                AddInternalProperties(ecmaVm, objRef, ArkInternalValueType::Entry, internalObjects);
                ArrayRef::SetValueAt(ecmaVm, jsValueRef, pos++, objRef);
            } else {
                ArrayRef::SetValueAt(ecmaVm, jsValueRef, pos++, JSNApiHelper::ToLocal<JSValueRef>(currentKey));
            }
        }
    }
    AddInternalProperties(ecmaVm, jsValueRef, ArkInternalValueType::Entry, internalObjects);
    return jsValueRef;
}

Local<JSValueRef> DebuggerApi::GetLightWeightMapValue(const EcmaVM *ecmaVm, Local<JSValueRef> value,
                                                      Global<MapRef> internalObjects)
{
    JSHandle<JSAPILightWeightMap> lightweightMap(JSNApiHelper::ToJSHandle(value));
    uint32_t size  = static_cast<uint32_t>(lightweightMap->GetSize());
    Local<JSValueRef> jsValueRef = ArrayRef::New(ecmaVm, size);
    JSThread *thread = ecmaVm->GetJSThread();
    JSMutableHandle<TaggedArray> keys(thread, lightweightMap->GetKeys(thread));
    JSMutableHandle<TaggedArray> values(thread, lightweightMap->GetValues(thread));
    JSMutableHandle<JSTaggedValue> currentKey(thread, JSTaggedValue::Undefined());
    JSMutableHandle<JSTaggedValue> currentValue(thread, JSTaggedValue::Undefined());
    Local<JSValueRef> jsKey = StringRef::NewFromUtf8(ecmaVm, "key");
    Local<JSValueRef> jsValue = StringRef::NewFromUtf8(ecmaVm, "value");
    uint32_t index = 0;
    while (index < size) {
        currentKey.Update(keys->Get(thread, index));
        currentValue.Update(values->Get(thread, index));
        Local<ObjectRef> objRef = ObjectRef::New(ecmaVm);
        objRef->Set(ecmaVm, jsKey, JSNApiHelper::ToLocal<JSValueRef>(currentKey));
        objRef->Set(ecmaVm, jsValue, JSNApiHelper::ToLocal<JSValueRef>(currentValue));
        AddInternalProperties(ecmaVm, objRef, ArkInternalValueType::Entry, internalObjects);
        ArrayRef::SetValueAt(ecmaVm, jsValueRef, index++, objRef);
    }
    AddInternalProperties(ecmaVm, jsValueRef, ArkInternalValueType::Entry, internalObjects);
    return jsValueRef;
}

Local<JSValueRef> DebuggerApi::GetLightWeightSetValue(const EcmaVM *ecmaVm, Local<JSValueRef> value,
                                                      Global<MapRef> internalObjects)
{
    JSHandle<JSAPILightWeightSet> lightWeightSet(JSNApiHelper::ToJSHandle(value));
    uint32_t size = static_cast<uint32_t>(lightWeightSet->GetSize());
    Local<JSValueRef> jsValueRef = ArrayRef::New(ecmaVm, size);
    JSThread *thread = ecmaVm->GetJSThread();
    JSMutableHandle<JSTaggedValue> currentValue(thread, JSTaggedValue::Undefined());
    Local<JSValueRef> jsValue = StringRef::NewFromUtf8(ecmaVm, "value");
    uint32_t index = 0;
    while (index < size) {
        currentValue.Update(lightWeightSet->GetValueAt(thread, index));
        if (currentValue->IsECMAObject()) {
            Local<ObjectRef> objRef = ObjectRef::New(ecmaVm);
            objRef->Set(ecmaVm, jsValue, JSNApiHelper::ToLocal<JSValueRef>(currentValue));
            AddInternalProperties(ecmaVm, objRef, ArkInternalValueType::Entry, internalObjects);
            ArrayRef::SetValueAt(ecmaVm, jsValueRef, index++, objRef);
        } else {
            ArrayRef::SetValueAt(ecmaVm, jsValueRef, index++, JSNApiHelper::ToLocal<JSValueRef>(currentValue));
        }
    }
    AddInternalProperties(ecmaVm, jsValueRef, ArkInternalValueType::Entry, internalObjects);
    return jsValueRef;
}

Local<JSValueRef> DebuggerApi::GetLinkedListValue(const EcmaVM *ecmaVm, Local<JSValueRef> value,
                                                  Global<MapRef> internalObjects)
{
    JSHandle<JSAPILinkedList> linkedList(JSNApiHelper::ToJSHandle(value));
    JSThread *thread = ecmaVm->GetJSThread();
    JSHandle<TaggedDoubleList> doubleList(thread, linkedList->GetDoubleList(thread));
    uint32_t size = static_cast<uint32_t>(linkedList->Length(thread));
    Local<JSValueRef> jsValueRef = ArrayRef::New(ecmaVm, size);
    JSMutableHandle<JSTaggedValue> currentValue(thread, JSTaggedValue::Undefined());
    int valueNode = TaggedDoubleList::ELEMENTS_START_INDEX;
    uint32_t index = 0;
    while (index < size) {
        valueNode = doubleList->GetNextDataIndex(valueNode);
        currentValue.Update(doubleList->GetElement(thread, valueNode));
        ArrayRef::SetValueAt(ecmaVm, jsValueRef, index++, JSNApiHelper::ToLocal<JSValueRef>(currentValue));
    }
    AddInternalProperties(ecmaVm, jsValueRef, ArkInternalValueType::Entry, internalObjects);
    return jsValueRef;
}

Local<JSValueRef> DebuggerApi::GetListValue(const EcmaVM *ecmaVm, Local<JSValueRef> value,
                                            Global<MapRef> internalObjects)
{
    JSHandle<JSAPIList> list(JSNApiHelper::ToJSHandle(value));
    JSThread *thread = ecmaVm->GetJSThread();
    JSHandle<TaggedSingleList> singleList(thread, list->GetSingleList(thread));
    uint32_t size = static_cast<uint32_t>(list->Length(thread));
    Local<JSValueRef> jsValueRef = ArrayRef::New(ecmaVm, size);
    JSMutableHandle<JSTaggedValue> currentValue(thread, JSTaggedValue::Undefined());
    int valueNode = TaggedDoubleList::ELEMENTS_START_INDEX;
    uint32_t index = 0;
    while (index < size) {
        valueNode = singleList->GetNextDataIndex(valueNode);
        currentValue.Update(singleList->GetElement(thread, valueNode));
        ArrayRef::SetValueAt(ecmaVm, jsValueRef, index++, JSNApiHelper::ToLocal<JSValueRef>(currentValue));
    }
    AddInternalProperties(ecmaVm, jsValueRef, ArkInternalValueType::Entry, internalObjects);
    return jsValueRef;
}

Local<JSValueRef> DebuggerApi::GetPlainArrayValue(const EcmaVM *ecmaVm, Local<JSValueRef> value,
                                                  Global<MapRef> internalObjects)
{
    JSHandle<JSAPIPlainArray> plainarray(JSNApiHelper::ToJSHandle(value));
    uint32_t size = static_cast<uint32_t>(plainarray->GetSize());
    Local<JSValueRef> jsValueRef = ArrayRef::New(ecmaVm, size);
    JSThread *thread = ecmaVm->GetJSThread();
    JSHandle<TaggedArray> keyArray(thread, plainarray->GetKeys(thread));
    JSHandle<TaggedArray> valueArray(thread, plainarray->GetValues(thread));
    JSMutableHandle<JSTaggedValue> currentKey(thread, JSTaggedValue::Undefined());
    JSMutableHandle<JSTaggedValue> currentValue(thread, JSTaggedValue::Undefined());
    Local<JSValueRef> jsKey = StringRef::NewFromUtf8(ecmaVm, "key");
    Local<JSValueRef> jsValue = StringRef::NewFromUtf8(ecmaVm, "value");
    uint32_t index = 0;
    while (index < size) {
        currentKey.Update(keyArray->Get(thread, index));
        currentValue.Update(valueArray->Get(thread, index));
        Local<ObjectRef> objRef = ObjectRef::New(ecmaVm);
        objRef->Set(ecmaVm, jsKey, JSNApiHelper::ToLocal<JSValueRef>(currentKey));
        objRef->Set(ecmaVm, jsValue, JSNApiHelper::ToLocal<JSValueRef>(currentValue));
        AddInternalProperties(ecmaVm, objRef, ArkInternalValueType::Entry, internalObjects);
        ArrayRef::SetValueAt(ecmaVm, jsValueRef, index++, objRef);
    }
    AddInternalProperties(ecmaVm, jsValueRef, ArkInternalValueType::Entry, internalObjects);
    return jsValueRef;
}

Local<JSValueRef> DebuggerApi::GetQueueValue(const EcmaVM *ecmaVm, Local<JSValueRef> value,
                                             Global<MapRef> internalObjects)
{
    JSHandle<JSAPIQueue> queue(JSNApiHelper::ToJSHandle(value));
    JSThread *thread = ecmaVm->GetJSThread();
    uint32_t size = static_cast<uint32_t>(queue->GetSize(thread));
    Local<JSValueRef> jsValueRef = ArrayRef::New(ecmaVm, size);
    JSMutableHandle<JSTaggedValue> currentValue(thread, JSTaggedValue::Undefined());
    uint32_t pos = 0;
    uint32_t index = 0;
    while (index < size) {
        currentValue.Update(queue->Get(thread, pos));
        pos = queue->GetNextPosition(thread, pos);
        ArrayRef::SetValueAt(ecmaVm, jsValueRef, index++, JSNApiHelper::ToLocal<JSValueRef>(currentValue));
    }
    AddInternalProperties(ecmaVm, jsValueRef, ArkInternalValueType::Entry, internalObjects);
    return jsValueRef;
}

Local<JSValueRef> DebuggerApi::GetStackValue(const EcmaVM *ecmaVm, Local<JSValueRef> value,
                                             Global<MapRef> internalObjects)
{
    JSHandle<JSAPIStack> stack(JSNApiHelper::ToJSHandle(value));
    uint32_t size = static_cast<uint32_t>(stack->GetSize() + 1);
    Local<JSValueRef> jsValueRef = ArrayRef::New(ecmaVm, size);
    JSThread *thread = ecmaVm->GetJSThread();
    JSMutableHandle<JSTaggedValue> currentValue(thread, JSTaggedValue::Undefined());
    uint32_t index = 0;
    while (index < size) {
        currentValue.Update(stack->Get(thread, index));
        ArrayRef::SetValueAt(ecmaVm, jsValueRef, index++, JSNApiHelper::ToLocal<JSValueRef>(currentValue));
    }
    AddInternalProperties(ecmaVm, jsValueRef, ArkInternalValueType::Entry, internalObjects);
    return jsValueRef;
}

Local<JSValueRef> DebuggerApi::GetTreeMapValue(const EcmaVM *ecmaVm, Local<JSValueRef> value,
                                               Global<MapRef> internalObjects)
{
    JSHandle<JSAPITreeMap> treeMap(JSNApiHelper::ToJSHandle(value));
    JSThread *thread = ecmaVm->GetJSThread();
    uint32_t size = static_cast<uint32_t>(treeMap->GetSize(thread));
    Local<JSValueRef> jsValueRef = ArrayRef::New(ecmaVm, size);
    JSMutableHandle<TaggedTreeMap> iteratedMap(thread, treeMap->GetTreeMap(thread));
    uint32_t elements = static_cast<uint32_t>(iteratedMap->NumberOfElements());
    JSHandle<TaggedArray> entries = TaggedTreeMap::GetArrayFromMap(thread, iteratedMap);
    JSMutableHandle<JSTaggedValue> currentKey(thread, JSTaggedValue::Undefined());
    JSMutableHandle<JSTaggedValue> currentValue(thread, JSTaggedValue::Undefined());
    Local<JSValueRef> jsKey = StringRef::NewFromUtf8(ecmaVm, "key");
    Local<JSValueRef> jsValue = StringRef::NewFromUtf8(ecmaVm, "value");
    uint32_t index = 0;
    while (index < elements) {
        int entriesIndex = entries->Get(thread, index).GetInt();
        currentKey.Update(iteratedMap->GetKey(thread, entriesIndex));
        currentValue.Update(iteratedMap->GetValue(thread, entriesIndex));
        Local<ObjectRef> objRef = ObjectRef::New(ecmaVm);
        objRef->Set(ecmaVm, jsKey, JSNApiHelper::ToLocal<JSValueRef>(currentKey));
        objRef->Set(ecmaVm, jsValue, JSNApiHelper::ToLocal<JSValueRef>(currentValue));
        AddInternalProperties(ecmaVm, objRef, ArkInternalValueType::Entry, internalObjects);
        ArrayRef::SetValueAt(ecmaVm, jsValueRef, index++, objRef);
    }
    AddInternalProperties(ecmaVm, jsValueRef, ArkInternalValueType::Entry, internalObjects);
    return jsValueRef;
}

Local<JSValueRef> DebuggerApi::GetTreeSetValue(const EcmaVM *ecmaVm, Local<JSValueRef> value,
                                               Global<MapRef> internalObjects)
{
    JSHandle<JSAPITreeSet> treeSet(JSNApiHelper::ToJSHandle(value));
    JSThread *thread = ecmaVm->GetJSThread();
    JSMutableHandle<TaggedTreeSet> iteratedSet(thread, treeSet->GetTreeSet(thread));
    uint32_t elements = static_cast<uint32_t>(iteratedSet->NumberOfElements());
    Local<JSValueRef> jsValueRef = ArrayRef::New(ecmaVm, elements);
    JSHandle<TaggedArray> entries = TaggedTreeSet::GetArrayFromSet(thread, iteratedSet);
    JSMutableHandle<JSTaggedValue> currentValue(thread, JSTaggedValue::Undefined());
    Local<JSValueRef> jsValue = StringRef::NewFromUtf8(ecmaVm, "value");
    uint32_t index = 0;
    while (index < elements) {
        int entriesIndex = entries->Get(thread, index).GetInt();
        currentValue.Update(iteratedSet->GetKey(thread, entriesIndex));
        if (currentValue->IsECMAObject()) {
            Local<ObjectRef> objRef = ObjectRef::New(ecmaVm);
            objRef->Set(ecmaVm, jsValue, JSNApiHelper::ToLocal<JSValueRef>(currentValue));
            AddInternalProperties(ecmaVm, objRef, ArkInternalValueType::Entry, internalObjects);
            ArrayRef::SetValueAt(ecmaVm, jsValueRef, index++, objRef);
        } else {
            ArrayRef::SetValueAt(ecmaVm, jsValueRef, index++, JSNApiHelper::ToLocal<JSValueRef>(currentValue));
        }
    }
    AddInternalProperties(ecmaVm, jsValueRef, ArkInternalValueType::Entry, internalObjects);
    return jsValueRef;
}

Local<JSValueRef> DebuggerApi::GetVectorValue(const EcmaVM *ecmaVm, Local<JSValueRef> value,
                                              Global<MapRef> internalObjects)
{
    JSHandle<JSAPIVector> vector(JSNApiHelper::ToJSHandle(value));
    uint32_t size = static_cast<uint32_t>(vector->GetSize());
    Local<JSValueRef> jsValueRef = ArrayRef::New(ecmaVm, size);
    JSThread *thread = ecmaVm->GetJSThread();
    JSMutableHandle<JSTaggedValue> currentValue(thread, JSTaggedValue::Undefined());
    uint32_t index = 0;
    while (index < size) {
        currentValue.Update(vector->Get(thread, vector, index));
        ArrayRef::SetValueAt(ecmaVm, jsValueRef, index++, JSNApiHelper::ToLocal<JSValueRef>(currentValue));
    }
    AddInternalProperties(ecmaVm, jsValueRef, ArkInternalValueType::Entry, internalObjects);
    return jsValueRef;
}

std::shared_ptr<AsyncStack> DebuggerApi::GetCurrentAsyncParent(const EcmaVM *ecmaVm)
{
    return ecmaVm->GetAsyncStackTrace()->GetCurrentAsyncParent();
}

bool DebuggerApi::CheckPromiseQueueSize(const EcmaVM *ecmaVm)
{
    auto *debuggerMgr = ecmaVm->GetJsDebuggerManager();
    uint32_t queueSizeEntry = debuggerMgr->GetPromiseQueueSizeRecordOfTopFrame();
    JSThread *thread = ecmaVm->GetJSThread();
    uint32_t queueSizeCurrent = job::MicroJobQueue::GetPromiseQueueSize(thread, ecmaVm->GetMicroJobQueue());
    return queueSizeEntry == queueSizeCurrent;
}

bool DebuggerApi::CheckIsSendableMethod(const EcmaVM *ecmaVm)
{
    auto *debuggerMgr = ecmaVm->GetJsDebuggerManager();
    return debuggerMgr->CheckIsSendableMethod();
}
bool DebuggerApi::IsMainThread()
{
    return JSThread::IsMainThread();
}

void DebuggerApi::DropLastFrame(const EcmaVM *ecmaVm)
{
    auto *debuggerMgr = ecmaVm->GetJsDebuggerManager();
    debuggerMgr->DropLastFrame();
}

Local<JSValueRef> DebuggerApi::GetCurrentGlobalEnv(const EcmaVM *ecmaVm, const FrameHandler *frameHandler)
{
    auto thread = ecmaVm->GetJSThread();
    JSTaggedValue lexicalEnv;
    if (frameHandler == nullptr) {
        lexicalEnv = thread->GetCurrentLexenv();
    } else {
        lexicalEnv = frameHandler->GetEnv();
    }
    JSHandle<JSTaggedValue> globalEnv(thread, thread->GetCurrentGlobalEnv(lexicalEnv));
    return JSNApiHelper::ToLocal<JSValueRef>(globalEnv);
}

DebuggerApi::DebuggerNativeScope::DebuggerNativeScope(const EcmaVM *vm)
{
    thread_ = vm->GetAssociatedJSThread();
    if (g_isEnableCMCGC) {
        hasSwitchState_ = thread_->GetThreadHolder()->TransferToNativeIfInRunning();
    } else {
        ecmascript::ThreadState oldState = thread_->GetState();
        if (oldState != ecmascript::ThreadState::RUNNING) {
            return;
        }
        oldThreadState_ = static_cast<uint16_t>(oldState);
        hasSwitchState_ = true;
        thread_->UpdateState(ecmascript::ThreadState::NATIVE);
    }
}

DebuggerApi::DebuggerNativeScope::~DebuggerNativeScope()
{
    if (hasSwitchState_) {
        if (g_isEnableCMCGC) {
            thread_->GetThreadHolder()->TransferToRunning();
        } else {
            thread_->UpdateState(static_cast<ecmascript::ThreadState>(oldThreadState_));
        }
    }
}

DebuggerApi::DebuggerManagedScope::DebuggerManagedScope(const EcmaVM *vm)
{
    thread_ = vm->GetAssociatedJSThread();
    if (g_isEnableCMCGC) {
        hasSwitchState_ = thread_->GetThreadHolder()->TransferToRunningIfInNative();
    } else {
        ecmascript::ThreadState oldState = thread_->GetState();
        if (oldState == ecmascript::ThreadState::RUNNING) {
            return;
        }
        oldThreadState_ = static_cast<uint16_t>(oldState);
        hasSwitchState_ = true;
        thread_->UpdateState(ecmascript::ThreadState::RUNNING);
    }
}

DebuggerApi::DebuggerManagedScope::~DebuggerManagedScope()
{
    if (hasSwitchState_) {
        if (g_isEnableCMCGC) {
            thread_->GetThreadHolder()->TransferToNative();
        } else {
            thread_->UpdateState(static_cast<ecmascript::ThreadState>(oldThreadState_));
        }
    }
}
}  // namespace panda::ecmascript::tooling
