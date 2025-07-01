/*
 * Copyright (c) 2023 Huawei Device Co., Ltd.
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

#include "ecmascript/debugger/dropframe_manager.h"

#include "ecmascript/interpreter/interpreter-inl.h"
#include "ecmascript/jobs/micro_job_queue.h"

namespace panda::ecmascript::tooling {
bool DropframeManager::IsNewlexenvOpcode(BytecodeInstruction::Opcode op)
{
    switch (op) {
        case BytecodeInstruction::Opcode::NEWLEXENV_IMM8:
        case BytecodeInstruction::Opcode::NEWLEXENVWITHNAME_IMM8_ID16:
        case BytecodeInstruction::Opcode::WIDE_NEWLEXENV_PREF_IMM16:
        case BytecodeInstruction::Opcode::WIDE_NEWLEXENVWITHNAME_PREF_IMM16_ID16:
            return true;
        default:
            break;
    }
    return false;
}

bool DropframeManager::IsStlexvarOpcode(BytecodeInstruction::Opcode op)
{
    switch (op) {
        case BytecodeInstruction::Opcode::STLEXVAR_IMM4_IMM4:
        case BytecodeInstruction::Opcode::STLEXVAR_IMM8_IMM8:
        case BytecodeInstruction::Opcode::WIDE_STLEXVAR_PREF_IMM16_IMM16:
            return true;
        default:
            break;
    }
    return false;
}

std::pair<uint16_t, uint16_t> DropframeManager::ReadStlexvarParams(const uint8_t *pc, BytecodeInstruction::Opcode op)
{
    uint16_t level = 0;
    uint16_t slot = 0;
    switch (op) {
        case BytecodeInstruction::Opcode::STLEXVAR_IMM4_IMM4:
            level = READ_INST_4_0();
            slot = READ_INST_4_1();
            break;
        case BytecodeInstruction::Opcode::STLEXVAR_IMM8_IMM8:
            level = READ_INST_8_0();
            slot = READ_INST_8_1();
            break;
        case BytecodeInstruction::Opcode::WIDE_STLEXVAR_PREF_IMM16_IMM16:
            level = READ_INST_16_1();
            slot = READ_INST_16_3();
            break;
        default:
            break;
    }
    return std::make_pair(level, slot);
}

void DropframeManager::MethodEntry(JSThread *thread, JSHandle<Method> method, JSHandle<JSTaggedValue> envHandle)
{
    std::set<std::pair<uint16_t, uint16_t>> modifiedLexVarPos;
    const JSPandaFile* methodJsPandaFile = method->GetJSPandaFile(thread);
    panda_file::File::EntityId methodId = method->GetMethodId();
    PushMethodInfo(std::make_tuple(const_cast<JSPandaFile *>(methodJsPandaFile), methodId));
    if (method->IsSendableMethod()) {
        PushMethodType(MethodType::SENDABLE_METHOD);
        return;
    }
    NewLexModifyRecordLevel();
    PushPromiseQueueSizeRecord(thread);
    if (!envHandle->IsLexicalEnv()) {
        PushMethodType(MethodType::OTHER_METHOD);
        return;
    }
    PushMethodType(MethodType::NORMAL_METHOD);
    uint32_t codeSize = method->GetCodeSize(thread);
    uint16_t newEnvCount = 0;
    auto bcIns = BytecodeInstruction(method->GetBytecodeArray());
    auto bcInsLast = bcIns.JumpTo(codeSize);
    while (bcIns.GetAddress() != bcInsLast.GetAddress()) {
        AddLexPropertiesToRecord(thread, bcIns, newEnvCount, modifiedLexVarPos, envHandle);
        bcIns = bcIns.GetNext();
    }
}

void DropframeManager::AddLexPropertiesToRecord(JSThread *thread, BytecodeInstruction &bcIns, uint16_t &newEnvCount,
    std::set<std::pair<uint16_t, uint16_t>> &modifiedLexVarPos, JSHandle<JSTaggedValue> envHandle)
{
    BytecodeInstruction::Opcode op = bcIns.GetOpcode();
    if (IsNewlexenvOpcode(op)) {
        newEnvCount++;
        return;
    }
    if (IsStlexvarOpcode(op)) {
        std::pair<uint16_t, uint16_t> lexVarPos = ReadStlexvarParams(bcIns.GetAddress(), op);
        uint16_t level;
        uint16_t slot;
        std::tie(level, slot) = lexVarPos;
        JSTaggedValue env = envHandle.GetTaggedValue();
        for (uint16_t i = 0; ; i++) {
            if ((level < newEnvCount || i >= level - newEnvCount) &&
                slot < LexicalEnv::Cast(env.GetTaggedObject())->GetLength() - LexicalEnv::RESERVED_ENV_LENGTH &&
                !modifiedLexVarPos.count({i, slot})) {
                JSTaggedValue value = LexicalEnv::Cast(env.GetTaggedObject())->GetProperties(thread, slot) ;
                EmplaceLexModifyRecord(thread, env, slot, value);
                modifiedLexVarPos.insert({i, slot});
            }
            if (i >= level) {
                break;
            }
            JSTaggedValue taggedParentEnv = LexicalEnv::Cast(env.GetTaggedObject())->GetParentEnv(thread);
            if (!taggedParentEnv.IsLexicalEnv()) {
                break;
            }
            env = taggedParentEnv;
        }
    }
}

void DropframeManager::MethodExit(JSThread *thread, [[maybe_unused]] JSHandle<Method> method)
{
    const JSPandaFile* methodJsPandaFile = method->GetJSPandaFile(thread);
    panda_file::File::EntityId methodId = method->GetMethodId();
    if (!CheckExitMethodInfo(std::make_tuple(const_cast<JSPandaFile *>(methodJsPandaFile), methodId))) {
        return;
    }
    PopMethodInfo();
    if (CheckIsSendableMethod()) {
        PopMethodType();
        return;
    }
    PopMethodType();
    MergeLexModifyRecordOfTopFrame(thread);
    PopPromiseQueueSizeRecord();
}

void DropframeManager::DropLastFrame(JSThread *thread)
{
    std::vector<std::tuple<JSHandle<JSTaggedValue>, uint16_t, JSHandle<JSTaggedValue>>> lexModifyRecord;
    lexModifyRecord = GetLexModifyRecordOfTopFrame();
    for (const auto &item : lexModifyRecord) {
        JSHandle<JSTaggedValue> envHandle;
        uint16_t slot;
        JSHandle<JSTaggedValue> valueHandle;
        std::tie(envHandle, slot, valueHandle) = item;
        JSTaggedValue env = envHandle.GetTaggedValue();
        ASSERT(slot < LexicalEnv::Cast(env.GetTaggedObject())->GetLength() - LexicalEnv::RESERVED_ENV_LENGTH);
        LexicalEnv::Cast(env.GetTaggedObject())->SetProperties(thread, slot, valueHandle.GetTaggedValue());
    }
    PopMethodInfo();
    PopMethodType();
    RemoveLexModifyRecordOfTopFrame(thread);
    PopPromiseQueueSizeRecord();

    FrameHandler frameHandler(thread);
    bool isEntryFrameDropped = false;
    while (frameHandler.HasFrame()) {
        frameHandler.PrevJSFrame();
        if (frameHandler.IsEntryFrame()) {
            isEntryFrameDropped = true;
            continue;
        }
        if (frameHandler.IsBuiltinFrame()) {
            continue;
        }
        if (!thread->IsAsmInterpreter()) {
            JSTaggedType *prevSp = frameHandler.GetSp();
            thread->SetCurrentFrame(prevSp);
        }
        if (isEntryFrameDropped) {
            thread->SetEntryFrameDroppedState();
        }
        thread->SetFrameDroppedState();
        break;
    }
}

void DropframeManager::NewLexModifyRecordLevel()
{
    modifiedLexVar_.push(std::vector<std::tuple<JSHandle<JSTaggedValue>, uint16_t, JSHandle<JSTaggedValue>>>());
}

void DropframeManager::EmplaceLexModifyRecord(JSThread *thread, JSTaggedValue env, uint16_t slot, JSTaggedValue value)
{
    GlobalHandleCollection globalHandleCollection(thread);
    for (const auto &item : modifiedLexVar_.top()) {
        JSHandle<JSTaggedValue> envHandleRecord = std::get<0>(item);
        uint16_t slotRecord = std::get<1>(item);
        if (envHandleRecord.GetTaggedType() == env.GetRawData() && slotRecord == slot) {
            return;
        }
    }
    JSHandle<JSTaggedValue> envHandle = globalHandleCollection.NewHandle<JSTaggedValue>(env.GetRawData());
    JSHandle<JSTaggedValue> valueHandle = globalHandleCollection.NewHandle<JSTaggedValue>(value.GetRawData());
    modifiedLexVar_.top().emplace_back(envHandle, slot, valueHandle);
}

uint32_t DropframeManager::GetLexModifyRecordLevel()
{
    return modifiedLexVar_.size();
}

std::vector<std::tuple<JSHandle<JSTaggedValue>, uint16_t, JSHandle<JSTaggedValue>>>
    DropframeManager::GetLexModifyRecordOfTopFrame()
{
    if (modifiedLexVar_.empty()) {
        return std::vector<std::tuple<JSHandle<JSTaggedValue>, uint16_t, JSHandle<JSTaggedValue>>>();
    }
    return modifiedLexVar_.top();
}

void DropframeManager::RemoveLexModifyRecordOfTopFrame(JSThread *thread)
{
    if (modifiedLexVar_.empty()) {
        return;
    }
    GlobalHandleCollection globalHandleCollection(thread);
    for (const auto &item : modifiedLexVar_.top()) {
        JSHandle<JSTaggedValue> envHandle = std::get<0>(item);
        JSHandle<JSTaggedValue> valueHandle = std::get<2>(item); // 2：get the third item of the tuple
        globalHandleCollection.Dispose(envHandle);
        globalHandleCollection.Dispose(valueHandle);
    }
    modifiedLexVar_.pop();
}

void DropframeManager::MergeLexModifyRecordOfTopFrame(JSThread *thread)
{
    if (modifiedLexVar_.empty()) {
        return;
    }
    GlobalHandleCollection globalHandleCollection(thread);
    std::vector<std::tuple<JSHandle<JSTaggedValue>, uint16_t, JSHandle<JSTaggedValue>>> lexModifyRecord;
    lexModifyRecord = modifiedLexVar_.top();
    modifiedLexVar_.pop();
    if (!modifiedLexVar_.empty() && modifiedLexVar_.top().empty()) {
        modifiedLexVar_.pop();
        modifiedLexVar_.push(lexModifyRecord);
        return;
    }
    for (const auto &item : lexModifyRecord) {
        JSHandle<JSTaggedValue> envHandle;
        uint16_t slot;
        JSHandle<JSTaggedValue> valueHandle;
        std::tie(envHandle, slot, valueHandle) = item;
        bool existRecord = false;
        if (!modifiedLexVar_.empty()) {
            for (const auto &itemLast : modifiedLexVar_.top()) {
                JSHandle<JSTaggedValue> envHandleRecord = std::get<0>(itemLast);
                uint16_t slotRecord = std::get<1>(itemLast);
                if (envHandleRecord.GetTaggedType() == envHandle.GetTaggedType() && slotRecord == slot) {
                    existRecord = true;
                    break;
                }
            }
        }
        if (modifiedLexVar_.empty() || existRecord) {
            globalHandleCollection.Dispose(envHandle);
            globalHandleCollection.Dispose(valueHandle);
        } else {
            modifiedLexVar_.top().emplace_back(envHandle, slot, valueHandle);
        }
    }
}

void DropframeManager::PushPromiseQueueSizeRecord(JSThread *thread)
{
    uint32_t queueSize = job::MicroJobQueue::GetPromiseQueueSize(thread, thread->GetEcmaVM()->GetMicroJobQueue());
    promiseQueueSizeRecord_.push(queueSize);
}

uint32_t DropframeManager::GetPromiseQueueSizeRecordOfTopFrame()
{
    ASSERT(!promiseQueueSizeRecord_.empty());
    return promiseQueueSizeRecord_.top();
}

void DropframeManager::PopPromiseQueueSizeRecord()
{
    if (!promiseQueueSizeRecord_.empty()) {
        promiseQueueSizeRecord_.pop();
    }
}

void DropframeManager::PushMethodInfo(std::tuple<JSPandaFile*,
    panda_file::File::EntityId> methodInfo)
{
    methodInfo_.push(methodInfo);
}

bool DropframeManager::CheckExitMethodInfo(std::tuple<JSPandaFile*,
    panda_file::File::EntityId> methodInfo)
{
    if (methodInfo_.empty()) {
        return false;
    }
    if (methodInfo == methodInfo_.top()) {
        return true;
    }
    return false;
}

void DropframeManager::PopMethodInfo()
{
    if (!methodInfo_.empty()) {
        methodInfo_.pop();
    }
}

void DropframeManager::PushMethodType(MethodType methodType)
{
    methodType_.push(methodType);
}

bool DropframeManager::CheckIsSendableMethod()
{
    ASSERT(!methodType_.empty());
    return methodType_.top() == MethodType::SENDABLE_METHOD;
}

void DropframeManager::PopMethodType()
{
    if (!methodType_.empty()) {
        methodType_.pop();
    }
}
}