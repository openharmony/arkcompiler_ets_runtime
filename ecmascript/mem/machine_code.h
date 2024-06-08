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
#ifndef ECMASCRIPT_MEM_MACHINE_CODE_H
#define ECMASCRIPT_MEM_MACHINE_CODE_H

#include "ecmascript/ecma_macros.h"
#include "ecmascript/js_tagged_value.h"
#include "ecmascript/mem/barriers.h"
#include "ecmascript/mem/tagged_object.h"
#include "ecmascript/mem/visitor.h"
#include "ecmascript/stackmap/ark_stackmap.h"
#include "ecmascript/method.h"

#include "libpandabase/macros.h"

namespace panda::ecmascript {
enum class MachineCodeType : uint8_t {
    BASELINE_CODE,
    FAST_JIT_CODE,
};

struct MachineCodeDesc {
    uintptr_t rodataAddrBeforeText {0};
    size_t rodataSizeBeforeText {0};
    uintptr_t rodataAddrAfterText {0};
    size_t rodataSizeAfterText {0};

    uintptr_t codeAddr {0};
    size_t codeSize {0};
    uintptr_t funcEntryDesAddr {0};
    size_t funcEntryDesSize {0};
    uintptr_t stackMapOrOffsetTableAddr {0};
    size_t stackMapOrOffsetTableSize {0};
    MachineCodeType codeType {MachineCodeType::FAST_JIT_CODE};
#ifdef CODE_SIGN_ENABLE
    uintptr_t codeSigner {0};
#endif
#ifdef ENABLE_JITFORT
    uintptr_t instructionsAddr {0};
    size_t instructionsSize {0};
#endif
};

class MachineCode;
using JitCodeMap = std::map<MachineCode*, std::string>;
using JitCodeMapVisitor = std::function<void(std::map<JSTaggedType, JitCodeMap*>&)>;

// BaselineCode object layout:
//                      +-----------------------------------+
//                      |              MarkWord             | 8 bytes
//      INS_SIZE_OFFSET +-----------------------------------+
//                      |          machine payload size     | 4 bytes
//                      +-----------------------------------+
//                      |          FuncEntryDesc size (0)   | 4 bytes
//                      +-----------------------------------+
//                      |          instructions size        | 4 bytes
//                      +-----------------------------------+
//                      |          instructions addr        | 8 bytes (if JitFort enabled)
//                      +-----------------------------------+
//                      |       nativePcOffsetTable size    | 4 bytes
//                      +-----------------------------------+
//                      |             func addr             | 8 bytes
//    PAYLOAD_OFFSET/   +-----------------------------------+
//                      |              ...                  |
//     INSTR_OFFSET     |                                   | if JitFort enabled, will be in JitFort space
//   (16 byte align)    |     machine instructions(text)    | instead for non-huge sized machine code objects
//                      |              ...                  | and pointed to by "instructions addr"
//                      +-----------------------------------+
//                      |                                   |
//                      |         nativePcOffsetTable       |
//                      |              ...                  |
//                      +-----------------------------------+
//==================================================================
// JitCode object layout:
//                      +-----------------------------------+
//                      |              MarkWord             | 8 bytes
//      INS_SIZE_OFFSET +-----------------------------------+
//                      |             OSR offset            | 4 bytes
//                      +-----------------------------------+
//                      |              OSR mask             | 4 bytes
//                      +-----------------------------------+
//                      |          machine payload size     | 4 bytes
//                      +-----------------------------------+
//                      |          FuncEntryDesc size       | 4 bytes
//                      +-----------------------------------+
//                      |          instructions size        | 4 bytes
//                      +-----------------------------------+
//                      |          instructions addr        | 8 bytes (if JitFort enabled)
//                      +-----------------------------------+
//                      |           stack map size          | 4 bytes
//                      +-----------------------------------+
//                      |             func addr             | 8 bytes
//       PAYLOAD_OFFSET +-----------------------------------+
//       (8 byte align) |           FuncEntryDesc           |
//                      |              ...                  |
//       INSTR_OFFSET   +-----------------------------------+
//       (16 byte align)|     machine instructions(text)    | if JitFort enabled, will be in JitFort space
//                      |              ...                  | instead for non-huge sized machine code objects
//      STACKMAP_OFFSET +-----------------------------------+ and pointed to by "instuctions addr"
//                      |            ArkStackMap            |
//                      |              ...                  |
//                      +-----------------------------------+
class MachineCode : public TaggedObject {
public:
    NO_COPY_SEMANTIC(MachineCode);
    NO_MOVE_SEMANTIC(MachineCode);
    static MachineCode *Cast(TaggedObject *object)
    {
        ASSERT(JSTaggedValue(object).IsMachineCodeObject());
        return static_cast<MachineCode *>(object);
    }

    static constexpr size_t INS_SIZE_OFFSET = TaggedObjectSize();
    ACCESSORS_PRIMITIVE_FIELD(OSROffset, int32_t, INS_SIZE_OFFSET, OSRMASK_OFFSET);
    // The high 16bit is used as the flag bit, and the low 16bit is used as the count of OSR execution times.
    ACCESSORS_PRIMITIVE_FIELD(OsrMask, uint32_t, OSRMASK_OFFSET, PAYLOADSIZE_OFFSET);
    ACCESSORS_PRIMITIVE_FIELD(PayLoadSizeInBytes, uint32_t, PAYLOADSIZE_OFFSET, FUNCENTRYDESSIZE_OFFSET);
    ACCESSORS_PRIMITIVE_FIELD(FuncEntryDesSize, uint32_t, FUNCENTRYDESSIZE_OFFSET, INSTRSIZ_OFFSET);
#ifdef ENABLE_JITFORT
    ACCESSORS_PRIMITIVE_FIELD(InstructionsSize, uint32_t, INSTRSIZ_OFFSET, INSTRADDR_OFFSET);
    ACCESSORS_PRIMITIVE_FIELD(InstructionsAddr, uint64_t, INSTRADDR_OFFSET, STACKMAP_OR_OFFSETTABLE_SIZE_OFFSET);
#else
    ACCESSORS_PRIMITIVE_FIELD(InstructionsSize, uint32_t, INSTRSIZ_OFFSET, STACKMAP_OR_OFFSETTABLE_SIZE_OFFSET);
#endif
    ACCESSORS_PRIMITIVE_FIELD(StackMapOrOffsetTableSize, uint32_t,
        STACKMAP_OR_OFFSETTABLE_SIZE_OFFSET, FUNCADDR_OFFSET);
    ACCESSORS_PRIMITIVE_FIELD(FuncAddr, uint64_t, FUNCADDR_OFFSET, PADDING_OFFSET);
    ACCESSORS_PRIMITIVE_FIELD(Padding, uint64_t, PADDING_OFFSET, LAST_OFFSET);
    DEFINE_ALIGN_SIZE(LAST_OFFSET);
    static constexpr size_t PAYLOAD_OFFSET = SIZE;
    static constexpr uint32_t DATA_ALIGN = 8;
    static constexpr uint32_t TEXT_ALIGN = 16;
    static constexpr int32_t INVALID_OSR_OFFSET = -1;
    static constexpr uint32_t OSR_EXECUTE_CNT_OFFSET = OSRMASK_OFFSET + 2;
    static constexpr uint16_t OSR_DEOPT_FLAG = 0x80;

    DECL_DUMP()

    uintptr_t GetFuncEntryDesAddress() const
    {
        uintptr_t paddingAddr = reinterpret_cast<const uintptr_t>(this) + PADDING_OFFSET;
        return IsAligned(paddingAddr, TEXT_ALIGN) ? paddingAddr : reinterpret_cast<const uintptr_t>(this) +
            PAYLOAD_OFFSET;
    }

    uintptr_t GetText() const
    {
#ifdef ENABLE_JITFORT
        return GetInstructionsAddr();
#else
        return GetFuncEntryDesAddress() + GetFuncEntryDesSize();
#endif
    }

    uint8_t *GetStackMapOrOffsetTableAddress() const
    {
#ifdef ENABLE_JITFORT
        // stackmap immediately follows FuncEntryDesc area
        return reinterpret_cast<uint8_t*>(GetFuncEntryDesAddress() + GetFuncEntryDesSize());
#else
        return reinterpret_cast<uint8_t*>(GetText() + GetInstructionsSize());
#endif
    }

    size_t GetTextSize() const
    {
        return GetInstructionsSize();
    }

    void SetData(const MachineCodeDesc &desc, JSHandle<Method> &method, size_t dataSize);

    template <VisitType visitType>
    void VisitRangeSlot(const EcmaObjectRangeVisitor &visitor)
    {
        if (visitType == VisitType::ALL_VISIT) {
            visitor(this, ObjectSlot(ToUintPtr(this)),
                ObjectSlot(ToUintPtr(this) + GetMachineCodeObjectSize()), VisitObjectArea::RAW_DATA);
        }
    }

    size_t GetMachineCodeObjectSize()
    {
        return SIZE + this->GetPayLoadSizeInBytes();
    }

    uint32_t GetInstructionSizeInBytes() const
    {
        return GetPayLoadSizeInBytes();
    }

    bool IsInText(const uintptr_t pc) const;
    uintptr_t GetFuncEntryDes() const;

    std::tuple<uint64_t, uint8_t *, int, kungfu::CalleeRegAndOffsetVec> CalCallSiteInfo(uintptr_t retAddr) const;

    void SetOsrDeoptFlag(bool isDeopt)
    {
        uint16_t flag = Barriers::GetValue<uint16_t>(this, OSRMASK_OFFSET);
        if (isDeopt) {
            flag |= OSR_DEOPT_FLAG;
        } else {
            flag &= (~OSR_DEOPT_FLAG);
        }
        Barriers::SetPrimitive(this, OSRMASK_OFFSET, flag);
    }

    void SetOsrExecuteCnt(uint16_t count)
    {
        Barriers::SetPrimitive(this, OSR_EXECUTE_CNT_OFFSET, count);
    }
private:
    void SetBaselineCodeData(const MachineCodeDesc &desc, JSHandle<Method> &method, size_t dataSize);
};
}  // namespace panda::ecmascript
#endif  // ECMASCRIPT_MEM_MACHINE_CODE_H
