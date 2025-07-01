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
#include "ecmascript/mem/mem.h"
#include "ecmascript/mem/tagged_object.h"
#include "ecmascript/mem/visitor.h"
#include "ecmascript/stackmap/ark_stackmap.h"
#include "ecmascript/method.h"
#include "ecmascript/mem/jit_fort_memdesc.h"
#include "ecmascript/deoptimizer/calleeReg.h"

#include "libpandabase/macros.h"

namespace panda::ecmascript {
enum class MachineCodeType : uint8_t {
    BASELINE_CODE,
    FAST_JIT_CODE,
};

enum class MachineCodeArchType : uint8_t {
    AArch64,
    X86,
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
    uintptr_t heapConstantTableAddr {0};
    size_t heapConstantTableSize {0};
    MachineCodeType codeType {MachineCodeType::FAST_JIT_CODE};
    MachineCodeArchType archType {MachineCodeArchType::X86};
#ifdef JIT_ENABLE_CODE_SIGN
    uintptr_t codeSigner {0};
#endif
    uintptr_t instructionsAddr {0};
    size_t instructionsSize {0};
    bool isHugeObj {false};
    uintptr_t hugeObjRegion {0};

    size_t codeSizeAlign {0};
    size_t rodataSizeBeforeTextAlign {0};
    size_t rodataSizeAfterTextAlign {0};
    size_t funcEntryDesSizeAlign {0};
    size_t stackMapSizeAlign {0};
    size_t heapConstantTableSizeAlign {0};
    MemDesc *memDesc {nullptr};
    bool isAsyncCompileMode {false};
};

class MachineCode;
using JitCodeVector = std::vector<std::tuple<MachineCode*, std::string, uintptr_t>>;
using JitCodeMapVisitor = std::function<void(std::map<JSTaggedType, JitCodeVector*>&)>;

// BaselineCode object layout:
//                      +-----------------------------------+
//                      |              MarkWord             | 8 bytes
//      INS_SIZE_OFFSET +-----------------------------------+
//                      |          machine payload size     | 4 bytes
//                      +-----------------------------------+
//                      |          instructions size        | 4 bytes
//                      +-----------------------------------+
//                      |          instructions addr        | 8 bytes (if JitFort enabled)
//                      +-----------------------------------+
//                      |       nativePcOffsetTable size    | 4 bytes
//                      +-----------------------------------+
//                      |             func addr             | 8 bytes
//                      +-----------------------------------+
//                      |        fp deltaprevframe sp       | 8 bytes
//                      +-----------------------------------+
//                      |             func size             | 4 bytes
//                      +-----------------------------------+
//                      |        callee register num        | 4 bytes
//                      +-----------------------------------+
//                      |                                   | 64 * 4 bytes (AMD64 or ARM64)
//                      |      callee reg2offset array      | or 32 * 4 bytes (others)
//                      |                                   |
//                      +-----------------------------------+
//                      |             bit field             | 4 bytes
//                      +-----------------------------------+
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
//                      |          instructions size        | 4 bytes
//                      +-----------------------------------+
//                      |          instructions addr        | 8 bytes (if JitFort enabled)
//                      +-----------------------------------+
//                      |           stack map size          | 4 bytes
//                      +-----------------------------------+
//                      |     heap constant table size      | 4 bytes
//                      +-----------------------------------+
//                      |     heap constant table addr      | 8 bytes
//                      +-----------------------------------+
//                      |             func addr             | 8 bytes
//                      +-----------------------------------+
//                      |        fp deltaprevframe sp       | 8 bytes
//                      +-----------------------------------+
//                      |             func size             | 4 bytes
//                      +-----------------------------------+
//                      |        callee register num        | 4 bytes
//                      +-----------------------------------+
//                      |                                   | 64 * 4 bytes (AMD64 or ARM64)
//                      |      callee reg2offset array      | or 32 * 4 bytes (others)
//                      |                                   |
//                      +-----------------------------------+
//                      |             bit field             | 4 bytes
//                      +-----------------------------------+
//                      |                                   |
//       INSTR_OFFSET   |                                   |
//       (16 byte align)|     machine instructions(text)    | if JitFort enabled, will be in JitFort space
//                      |              ...                  | instead for non-huge sized machine code objects
//      STACKMAP_OFFSET +-----------------------------------+ and pointed to by "instuctions addr"
//                      |            ArkStackMap            |
//                      |              ...                  |
//                      +-----------------------------------+
//
//==================================================================
// HugeMachineCode object layout for JitFort (see space.cpp AllocateFort())
//
//   ^                  +-----------------------------------+          +
//   |                  |              MarkWord             |          |
//   |                  +-----------------------------------+          |
//   |                  |             OSR offset            |          |
//   |                  +-----------------------------------+          |
//   |                  |               ...                 |          |
//   |                  |                                   |          |
// mutable              +-----------------------------------+          |
// Page aligned         |          instructions size        |          |
//   |                  +-----------------------------------+          |
//   |                  |          instructions addr        |---+      |
//   |                  +-----------------------------------+   |      | 256 kByte (Region)
//   |                  |               ...                 |   |      |      multiples
//   |                  |                                   |   |      |
//   |                  +-----------------------------------+   |      |  if JitFort is disabled
//   |                  |             ArkStackMap           |   |      |  Jit generated native code
//   |                  |                                   |   |      |  location is placed between
//   v     Page Aligned +-----------------------------------+   |      |  FuncEntryDesc and ArkStackMap
//   ^     if JitFort   |                                   |<--+      |  instead and is mutable
//   |     enabled      |            JitFort space          |          |
//   |                  |    (Jit generated native code)    |          |
// immutable            |                                   |          |
// JitFort space        |                                   |          |
//   |                  |                                   |          |
//   |                  |                                   |          |
//   |                  |                                   |          |
//   v                  +-----------------------------------+          v
//
//
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
    static constexpr size_t INT32_SIZE = sizeof(int32_t);
    static constexpr int CalleeReg2OffsetArraySize = 2 * kungfu::MAX_CALLEE_SAVE_REIGISTER_NUM;
    ACCESSORS_PRIMITIVE_FIELD(LocalHeapAddress, uint64_t, INS_SIZE_OFFSET, LOCAL_HEAP_ADDRESS_OFFSET);
    ACCESSORS_PRIMITIVE_FIELD(OSROffset, int32_t, LOCAL_HEAP_ADDRESS_OFFSET, OSRMASK_OFFSET);
    // The high 16bit is used as the flag bit, and the low 16bit is used as the count of OSR execution times.
    ACCESSORS_PRIMITIVE_FIELD(OsrMask, uint32_t, OSRMASK_OFFSET, PAYLOADSIZE_OFFSET);
    ACCESSORS_PRIMITIVE_FIELD(PayLoadSizeInBytes, uint32_t, PAYLOADSIZE_OFFSET, INSTRSIZ_OFFSET);
    ACCESSORS_PRIMITIVE_FIELD(InstructionsSize, uint32_t, INSTRSIZ_OFFSET, INSTRADDR_OFFSET);
    ACCESSORS_PRIMITIVE_FIELD(InstructionsAddr, uint64_t, INSTRADDR_OFFSET, STACKMAP_OR_OFFSETTABLE_SIZE_OFFSET);
    ACCESSORS_PRIMITIVE_FIELD(StackMapOrOffsetTableSize, uint32_t,
        STACKMAP_OR_OFFSETTABLE_SIZE_OFFSET, HEAP_CONSTANT_TABLE_SIZE_OFFSET);
    ACCESSORS_PRIMITIVE_FIELD(HeapConstantTableSize, uint32_t,
        HEAP_CONSTANT_TABLE_SIZE_OFFSET, HEAP_CONSTANT_TABLE_ADDR_OFFSET);
    ACCESSORS_PRIMITIVE_FIELD(HeapConstantTableAddr, uint64_t,
        HEAP_CONSTANT_TABLE_ADDR_OFFSET, FUNCADDR_OFFSET);
    ACCESSORS_PRIMITIVE_FIELD(FuncAddr, uint64_t, FUNCADDR_OFFSET, FPDELTA_PRVE_FRAME_SP_OFFSET);
    ACCESSORS_PRIMITIVE_FIELD(FpDeltaPrevFrameSp, uintptr_t, FPDELTA_PRVE_FRAME_SP_OFFSET, FUNC_SIZE_OFFSET);
    ACCESSORS_PRIMITIVE_FIELD(FuncSize, uint32_t, FUNC_SIZE_OFFSET, CALLEE_REGISTERNUM_OFFSET);
    ACCESSORS_PRIMITIVE_FIELD(CalleeRegisterNum, uint32_t, CALLEE_REGISTERNUM_OFFSET, CALLEE_R2O_OFFSET);
    static constexpr size_t BIT_FIELD_OFFSET = CALLEE_R2O_OFFSET + INT32_SIZE * CalleeReg2OffsetArraySize;
    ACCESSORS_BIT_FIELD(BitField, BIT_FIELD_OFFSET, LAST_OFFSET);
    DEFINE_ALIGN_SIZE(LAST_OFFSET);
    static constexpr size_t PAYLOAD_OFFSET = SIZE;
    static constexpr uint32_t DATA_ALIGN = 8;
    static constexpr uint32_t TEXT_ALIGN = 16;
    static constexpr int32_t INVALID_OSR_OFFSET = -1;
    static constexpr uint32_t OSR_EXECUTE_CNT_OFFSET = OSRMASK_OFFSET + 2;
    static constexpr uint16_t OSR_DEOPT_FLAG = 0x80;

    void SetCalleeReg2OffsetArray(int32_t* calleeRegArray)
    {
        DASSERT_PRINT(calleeRegArray != nullptr, "Array pointer cannot be null.");
        for (size_t i = 0; i < CalleeReg2OffsetArraySize; i++) {
            Barriers::SetPrimitive<int32_t>(this, CALLEE_R2O_OFFSET + i * INT32_SIZE, calleeRegArray[i]);
        }
    }

    int32_t GetCalleeReg2OffsetArray(int32_t calleeRegIndex) const
    {
        DASSERT_PRINT(calleeRegIndex < CalleeReg2OffsetArraySize, "Array index out of bounds.");
        return Barriers::GetPrimitive<int32_t>(this, CALLEE_R2O_OFFSET + calleeRegIndex * INT32_SIZE);
    }

    // define BitField
    static constexpr size_t IS_FAST_CALL_BITS = 1;
    FIRST_BIT_FIELD(BitField, IsFastCall, bool, IS_FAST_CALL_BITS);

    DECL_DUMP()

    uintptr_t GetNonTextAddress() const
    {
        return reinterpret_cast<const uintptr_t>(this) + SIZE;
    }

    uintptr_t GetText() const;
    uint8_t *GetStackMapOrOffsetTableAddress() const;
    uint8_t *GetHeapConstantTableAddress() const;

    size_t GetTextSize() const
    {
        return GetInstructionsSize();
    }

    bool SetData(JSThread *thread, const MachineCodeDesc &desc, JSHandle<Method> &method, size_t dataSize,
                 RelocMap &relocInfo);
    bool SetText(const MachineCodeDesc &desc);
    bool SetNonText(const MachineCodeDesc &desc, EntityId methodId);

    template <VisitType visitType, class DerivedVisitor>
    void VisitRangeSlot(BaseObjectVisitor<DerivedVisitor> &visitor)
    {
        ASSERT(visitType == VisitType::ALL_VISIT || visitType == VisitType::OLD_GC_VISIT);
        if constexpr (visitType == VisitType::ALL_VISIT) {
            visitor(this, ToUintPtr(this),
                ToUintPtr(this) + GetMachineCodeObjectSize(), VisitObjectArea::RAW_DATA);
        }
        if constexpr (visitType == VisitType::OLD_GC_VISIT) {
            visitor(this,
                    static_cast<uintptr_t>(GetHeapConstantTableAddr()),
                    static_cast<uintptr_t>(GetHeapConstantTableAddr()) + GetHeapConstantTableSize(),
                    VisitObjectArea::NORMAL);
            this->ProcessMarkObject();
        }
    }

    void ProcessMarkObject();

    size_t GetMachineCodeObjectSize()
    {
        return SIZE + this->GetPayLoadSizeInBytes();
    }

    uint32_t GetInstructionSizeInBytes() const
    {
        return GetPayLoadSizeInBytes();
    }

    bool IsInText(const uintptr_t pc) const;

    std::tuple<uint64_t, uint8_t *, int, kungfu::CalleeRegAndOffsetVec> CalCallSiteInfo() const;

    void SetOsrDeoptFlag(bool isDeopt)
    {
        uint16_t flag = Barriers::GetPrimitive<uint16_t>(this, OSRMASK_OFFSET);
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
    bool SetBaselineCodeData(JSThread *thread, const MachineCodeDesc &desc, JSHandle<Method> &method,
                             size_t dataSize, RelocMap &relocInfo);
};
}  // namespace panda::ecmascript
#endif  // ECMASCRIPT_MEM_MACHINE_CODE_H
