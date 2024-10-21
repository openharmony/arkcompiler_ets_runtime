/*
 * Copyright (c) 2023-2024 Huawei Device Co., Ltd.
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

#include "ecmascript/compiler/pgo_bc_info.h"
#include "ecmascript/compiler/bytecodes.h"

namespace panda::ecmascript::kungfu {
void PGOBCInfo::Info::Record(const InfoDetail &detail)
{
    auto it = methodOffsetToValVec_.find(detail.methodOffset);
    if (it == methodOffsetToValVec_.end()) {
        methodOffsetToValVec_[detail.methodOffset] =
            ValVec { Val { detail.bcIndex, detail.bcOffset, detail.cpIndex} };
    } else {
        it->second.emplace_back(Val{ detail.bcIndex, detail.bcOffset, detail.cpIndex });
    }
    recordNameToValCount_[detail.recordName]++;
}

uint32_t PGOBCInfo::Info::GetPGOExtendGTCount(const CString &recordName) const
{
    auto it = recordNameToValCount_.find(recordName);
    if (it != recordNameToValCount_.end()) {
        return it->second;
    }
    return 0;
}

const PGOBCInfo::Info& PGOBCInfo::GetInfo(Type type) const
{
    ASSERT(Type::TYPE_FIRST <= type && type <= Type::TYPE_LAST);
    return infos_[type];
}

uint32_t PGOBCInfo::GetPGOExtendGTCount(const CString &recordName) const
{
    uint32_t count = 0;
    for (const Info &info : infos_) {
        count += info.GetPGOExtendGTCount(recordName);
    }
    return count;
}

void PGOBCInfo::Record(const InfoDetail &detail, Type type)
{
    ASSERT(Type::TYPE_FIRST <= type && type <= Type::TYPE_LAST);
    Info &info = infos_[type];
    info.Record(detail);
}

void PGOBCInfo::Record(const BytecodeInstruction &bcIns, int32_t bcIndex,
                       const CString &recordName, const MethodLiteral *method)
{
    BytecodeInstruction::Opcode opcode = static_cast<BytecodeInstruction::Opcode>(bcIns.GetOpcode());
    uint32_t methodOffset = method->GetMethodId().GetOffset();
    uint32_t bcOffset = bcIns.GetAddress() - method->GetBytecodeArray();
    if (Bytecodes::IsCreateObjectWithBufferOp(opcode)) {
        auto cpIndex = bcIns.GetId().AsRawValue();
        Record(InfoDetail {recordName, methodOffset, bcIndex, bcOffset, cpIndex}, Type::OBJ_LITERAL);
    } else if (Bytecodes::IsCreateArrayWithBufferOp(opcode)) {
        auto cpIndex = bcIns.GetId().AsRawValue();
        Record(InfoDetail {recordName, methodOffset, bcIndex, bcOffset, cpIndex}, Type::ARRAY_LITERAL);
    } else if (Bytecodes::IsCreateEmptyArrayOp(opcode)) {
        Record(InfoDetail {recordName, methodOffset, bcIndex, bcOffset, 0}, Type::EMPTY_ARRAY);
    } else if (Bytecodes::IsCallOp(opcode)) {
        Record(InfoDetail {recordName, methodOffset, bcIndex, bcOffset, 0}, Type::CALL_TARGET);
    } else if (Bytecodes::IsDefineClassWithBufferOp(opcode)) {
        Record(InfoDetail {recordName, methodOffset, bcIndex, bcOffset, 0}, Type::CLASS);
    } else if (Bytecodes::IsDefineFunc(opcode)) {
        Record(InfoDetail {recordName, methodOffset, bcIndex, bcOffset, 0}, Type::FUNCTION);
    }
}
}  // namespace panda::ecmascript
