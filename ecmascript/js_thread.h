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

#ifndef ECMASCRIPT_JS_THREAD_H
#define ECMASCRIPT_JS_THREAD_H

#include <atomic>
#include <sstream>

#include "ecmascript/base/aligned_struct.h"
#include "ecmascript/compiler/builtins/builtins_call_signature.h"
#include "ecmascript/compiler/common_stubs.h"
#include "ecmascript/compiler/interpreter_stub.h"
#include "ecmascript/compiler/rt_call_signature.h"
#include "ecmascript/dfx/vm_thread_control.h"
#include "ecmascript/elements.h"
#include "ecmascript/frames.h"
#include "ecmascript/global_env_constants.h"
#include "ecmascript/mem/visitor.h"

namespace panda::ecmascript {
class EcmaContext;
class EcmaVM;
class EcmaHandleScope;
class HeapRegionAllocator;
class PropertiesCache;
template<typename T>
class EcmaGlobalStorage;
class Node;
class DebugNode;
class VmThreadControl;
using WeakClearCallback = void (*)(void *);

enum class MarkStatus : uint8_t {
    READY_TO_MARK,
    MARKING,
    MARK_FINISHED,
};

enum class PGOProfilerStatus : uint8_t {
    PGO_PROFILER_DISABLE,
    PGO_PROFILER_ENABLE,
};

enum class BCStubStatus: uint8_t {
    NORMAL_BC_STUB,
    PROFILE_BC_STUB,
};

enum class StableArrayChangeKind { PROTO, NOT_PROTO };

struct BCStubEntries {
    static constexpr size_t EXISTING_BC_HANDLER_STUB_ENTRIES_COUNT =
        kungfu::BytecodeStubCSigns::NUM_OF_ALL_NORMAL_STUBS;
    // The number of bytecodes.
    static constexpr size_t BC_HANDLER_COUNT = kungfu::BytecodeStubCSigns::LAST_VALID_OPCODE + 1;
    static constexpr size_t COUNT = kungfu::BytecodeStubCSigns::NUM_OF_STUBS;
    static_assert(EXISTING_BC_HANDLER_STUB_ENTRIES_COUNT <= COUNT);
    Address stubEntries_[COUNT] = {0};

    static constexpr size_t SizeArch32 = sizeof(uint32_t) * COUNT;
    static constexpr size_t SizeArch64 = sizeof(uint64_t) * COUNT;

    void Set(size_t index, Address addr)
    {
        ASSERT(index < COUNT);
        stubEntries_[index] = addr;
    }

    Address* GetAddr()
    {
        return reinterpret_cast<Address*>(stubEntries_);
    }

    Address Get(size_t index) const
    {
        ASSERT(index < COUNT);
        return stubEntries_[index];
    }
};
STATIC_ASSERT_EQ_ARCH(sizeof(BCStubEntries), BCStubEntries::SizeArch32, BCStubEntries::SizeArch64);

struct RTStubEntries {
    static constexpr size_t COUNT = kungfu::RuntimeStubCSigns::NUM_OF_STUBS;
    Address stubEntries_[COUNT];

    static constexpr size_t SizeArch32 = sizeof(uint32_t) * COUNT;
    static constexpr size_t SizeArch64 = sizeof(uint64_t) * COUNT;

    void Set(size_t index, Address addr)
    {
        ASSERT(index < COUNT);
        stubEntries_[index] = addr;
    }

    Address Get(size_t index) const
    {
        ASSERT(index < COUNT);
        return stubEntries_[index];
    }
};
STATIC_ASSERT_EQ_ARCH(sizeof(RTStubEntries), RTStubEntries::SizeArch32, RTStubEntries::SizeArch64);

struct COStubEntries {
    static constexpr size_t COUNT = kungfu::CommonStubCSigns::NUM_OF_STUBS;
    Address stubEntries_[COUNT];

    static constexpr size_t SizeArch32 = sizeof(uint32_t) * COUNT;
    static constexpr size_t SizeArch64 = sizeof(uint64_t) * COUNT;

    void Set(size_t index, Address addr)
    {
        ASSERT(index < COUNT);
        stubEntries_[index] = addr;
    }

    Address Get(size_t index) const
    {
        ASSERT(index < COUNT);
        return stubEntries_[index];
    }
};

struct BCDebuggerStubEntries {
    static constexpr size_t EXISTING_BC_HANDLER_STUB_ENTRIES_COUNT =
        kungfu::BytecodeStubCSigns::NUM_OF_ALL_NORMAL_STUBS;
    static constexpr size_t COUNT = kungfu::BytecodeStubCSigns::LAST_VALID_OPCODE + 1;
    Address stubEntries_[COUNT];

    static constexpr size_t SizeArch32 = sizeof(uint32_t) * COUNT;
    static constexpr size_t SizeArch64 = sizeof(uint64_t) * COUNT;

    void Set(size_t index, Address addr)
    {
        ASSERT(index < COUNT);
        stubEntries_[index] = addr;
    }

    Address Get(size_t index) const
    {
        ASSERT(index < COUNT);
        return stubEntries_[index];
    }

    void SetNonexistentBCHandlerStubEntries(Address addr)
    {
        for (size_t i = EXISTING_BC_HANDLER_STUB_ENTRIES_COUNT; i < COUNT; i++) {
            if (stubEntries_[i] == 0) {
                stubEntries_[i] = addr;
            }
        }
    }
};

struct BuiltinStubEntries {
    static constexpr size_t COUNT = kungfu::BuiltinsStubCSigns::NUM_OF_BUILTINS_STUBS;
    Address stubEntries_[COUNT];

    static constexpr size_t SizeArch32 = sizeof(uint32_t) * COUNT;
    static constexpr size_t SizeArch64 = sizeof(uint64_t) * COUNT;

    void Set(size_t index, Address addr)
    {
        ASSERT(index < COUNT);
        stubEntries_[index] = addr;
    }

    Address Get(size_t index) const
    {
        ASSERT(index < COUNT);
        return stubEntries_[index];
    }
};
STATIC_ASSERT_EQ_ARCH(sizeof(COStubEntries), COStubEntries::SizeArch32, COStubEntries::SizeArch64);

class JSThread {
public:
    static constexpr int CONCURRENT_MARKING_BITFIELD_NUM = 2;
    static constexpr int CHECK_SAFEPOINT_BITFIELD_NUM = 8;
    static constexpr int PGO_PROFILER_BITFIELD_START = 16;
    static constexpr int BOOL_BITFIELD_NUM = 1;
    static constexpr uint32_t RESERVE_STACK_SIZE = 128;
    static constexpr uint64_t CHECKSAFEPOINT_FLAG = 0xFF;
    using MarkStatusBits = BitField<MarkStatus, 0, CONCURRENT_MARKING_BITFIELD_NUM>;
    using CheckSafePointBit = BitField<bool, 0, BOOL_BITFIELD_NUM>;
    using VMNeedSuspensionBit = BitField<bool, CHECK_SAFEPOINT_BITFIELD_NUM, BOOL_BITFIELD_NUM>;
    using VMHasSuspendedBit = VMNeedSuspensionBit::NextFlag;
    using PGOStatusBits = BitField<PGOProfilerStatus, PGO_PROFILER_BITFIELD_START, BOOL_BITFIELD_NUM>;
    using BCStubStatusBits = PGOStatusBits::NextField<BCStubStatus, BOOL_BITFIELD_NUM>;
    using ThreadId = uint32_t;

    enum FrameDroppedState {
        StateFalse = 0,
        StateTrue,
        StatePending
    };

    explicit JSThread(EcmaVM *vm);

    PUBLIC_API ~JSThread();

    EcmaVM *GetEcmaVM() const
    {
        return vm_;
    }

    static JSThread *Create(EcmaVM *vm);

    int GetNestedLevel() const
    {
        return nestedLevel_;
    }

    void SetNestedLevel(int level)
    {
        nestedLevel_ = level;
    }

    void SetLastFp(JSTaggedType *fp)
    {
        glueData_.lastFp_ = fp;
    }

    const JSTaggedType *GetLastFp() const
    {
        return glueData_.lastFp_;
    }

    const JSTaggedType *GetCurrentSPFrame() const
    {
        return glueData_.currentFrame_;
    }

    void SetCurrentSPFrame(JSTaggedType *sp)
    {
        glueData_.currentFrame_ = sp;
    }

    const JSTaggedType *GetLastLeaveFrame() const
    {
        return glueData_.leaveFrame_;
    }

    void SetLastLeaveFrame(JSTaggedType *sp)
    {
        glueData_.leaveFrame_ = sp;
    }

    const JSTaggedType *GetCurrentFrame() const;

    void SetCurrentFrame(JSTaggedType *sp);

    const JSTaggedType *GetCurrentInterpretedFrame() const;

    bool DoStackOverflowCheck(const JSTaggedType *sp);

    NativeAreaAllocator *GetNativeAreaAllocator() const
    {
        return nativeAreaAllocator_;
    }

    HeapRegionAllocator *GetHeapRegionAllocator() const
    {
        return heapRegionAllocator_;
    }

    void ReSetNewSpaceAllocationAddress(const uintptr_t *top, const uintptr_t* end)
    {
        glueData_.newSpaceAllocationTopAddress_ = top;
        glueData_.newSpaceAllocationEndAddress_ = end;
    }

    void SetIsStartHeapSampling(bool isStart)
    {
        glueData_.isStartHeapSampling_ = isStart ? JSTaggedValue::True() : JSTaggedValue::False();
    }

    void Iterate(const RootVisitor &visitor, const RootRangeVisitor &rangeVisitor,
        const RootBaseAndDerivedVisitor &derivedVisitor);

    void IterateHandleWithCheck(const RootVisitor &visitor, const RootRangeVisitor &rangeVisitor);

    uintptr_t* PUBLIC_API ExpandHandleStorage();
    void PUBLIC_API ShrinkHandleStorage(int prevIndex);
    void PUBLIC_API CheckJSTaggedType(JSTaggedType value) const;
    bool PUBLIC_API CpuProfilerCheckJSTaggedType(JSTaggedType value) const;

    std::vector<std::pair<WeakClearCallback, void *>> *GetWeakNodeNativeFinalizeCallbacks()
    {
        return &weakNodeNativeFinalizeCallbacks_;
    }

    void SetException(JSTaggedValue exception);

    JSTaggedValue GetException() const
    {
        return glueData_.exception_;
    }

    bool HasPendingException() const
    {
        return !glueData_.exception_.IsHole();
    }

    void ClearException();

    void SetGlobalObject(JSTaggedValue globalObject)
    {
        glueData_.globalObject_ = globalObject;
    }

    const GlobalEnvConstants *GlobalConstants() const
    {
        return glueData_.globalConst_;
    }

    const CMap<ElementsKind, ConstantIndex> &GetArrayHClassIndexMap() const
    {
        return arrayHClassIndexMap_;
    }

    void NotifyStableArrayElementsGuardians(JSHandle<JSObject> receiver, StableArrayChangeKind changeKind);

    bool IsStableArrayElementsGuardiansInvalid() const
    {
        return !glueData_.stableArrayElementsGuardians_;
    }

    void ResetGuardians();

    JSTaggedValue GetCurrentLexenv() const;

    void RegisterRTInterface(size_t id, Address addr)
    {
        ASSERT(id < kungfu::RuntimeStubCSigns::NUM_OF_STUBS);
        glueData_.rtStubEntries_.Set(id, addr);
    }

    Address GetRTInterface(size_t id) const
    {
        ASSERT(id < kungfu::RuntimeStubCSigns::NUM_OF_STUBS);
        return glueData_.rtStubEntries_.Get(id);
    }

    Address GetFastStubEntry(uint32_t id) const
    {
        return glueData_.coStubEntries_.Get(id);
    }

    void SetFastStubEntry(size_t id, Address entry)
    {
        glueData_.coStubEntries_.Set(id, entry);
    }

    Address GetBuiltinStubEntry(uint32_t id) const
    {
        return glueData_.builtinStubEntries_.Get(id);
    }

    void SetBuiltinStubEntry(size_t id, Address entry)
    {
        glueData_.builtinStubEntries_.Set(id, entry);
    }

    Address GetBCStubEntry(uint32_t id) const
    {
        return glueData_.bcStubEntries_.Get(id);
    }

    void SetBCStubEntry(size_t id, Address entry)
    {
        glueData_.bcStubEntries_.Set(id, entry);
    }

    void SetBCDebugStubEntry(size_t id, Address entry)
    {
        glueData_.bcDebuggerStubEntries_.Set(id, entry);
    }

    Address *GetBytecodeHandler()
    {
        return glueData_.bcStubEntries_.GetAddr();
    }

    void PUBLIC_API CheckSwitchDebuggerBCStub();
    void CheckOrSwitchPGOStubs();

    ThreadId GetThreadId() const
    {
        return id_.load(std::memory_order_relaxed);
    }

    void SetThreadId()
    {
        id_.store(JSThread::GetCurrentThreadId(), std::memory_order_relaxed);
    }

    static ThreadId GetCurrentThreadId()
    {
        return os::thread::GetCurrentThreadId();
    }

    void IterateWeakEcmaGlobalStorage(const WeakRootVisitor &visitor);

    PropertiesCache *GetPropertiesCache() const;

    void SetMarkStatus(MarkStatus status)
    {
        MarkStatusBits::Set(status, &glueData_.gcStateBitField_);
    }

    bool IsReadyToMark() const
    {
        auto status = MarkStatusBits::Decode(glueData_.gcStateBitField_);
        return status == MarkStatus::READY_TO_MARK;
    }

    bool IsMarking() const
    {
        auto status = MarkStatusBits::Decode(glueData_.gcStateBitField_);
        return status == MarkStatus::MARKING;
    }

    bool IsMarkFinished() const
    {
        auto status = MarkStatusBits::Decode(glueData_.gcStateBitField_);
        return status == MarkStatus::MARK_FINISHED;
    }

    void SetPGOProfilerEnable(bool enable)
    {
        PGOProfilerStatus status =
            enable ? PGOProfilerStatus::PGO_PROFILER_ENABLE : PGOProfilerStatus::PGO_PROFILER_DISABLE;
        PGOStatusBits::Set(status, &glueData_.interruptVector_);
    }

    bool IsPGOProfilerEnable() const
    {
        auto status = PGOStatusBits::Decode(glueData_.interruptVector_);
        return status == PGOProfilerStatus::PGO_PROFILER_ENABLE;
    }

    void SetBCStubStatus(BCStubStatus status)
    {
        BCStubStatusBits::Set(status, &glueData_.interruptVector_);
    }

    BCStubStatus GetBCStubStatus() const
    {
        return BCStubStatusBits::Decode(glueData_.interruptVector_);
    }

    bool CheckSafepoint();

    void SetGetStackSignal(bool isParseStack)
    {
        getStackSignal_ = isParseStack;
    }

    bool GetStackSignal() const
    {
        return getStackSignal_;
    }

    void SetNeedProfiling(bool needProfiling)
    {
        needProfiling_.store(needProfiling);
    }

    void SetIsProfiling(bool isProfiling)
    {
        isProfiling_ = isProfiling;
    }

    bool GetIsProfiling()
    {
        return isProfiling_;
    }

    void SetGcState(bool gcState)
    {
        gcState_ = gcState;
    }

    bool GetGcState() const
    {
        return gcState_;
    }

    void SetRuntimeState(bool runtimeState)
    {
        runtimeState_ = runtimeState;
    }

    bool GetRuntimeState() const
    {
        return runtimeState_;
    }

    void SetCpuProfileName(std::string &profileName)
    {
        profileName_ = profileName;
    }

    void EnableAsmInterpreter()
    {
        isAsmInterpreter_ = true;
    }

    bool IsAsmInterpreter() const
    {
        return isAsmInterpreter_;
    }

    VmThreadControl *GetVmThreadControl() const
    {
        return vmThreadControl_;
    }

    static constexpr size_t GetGlueDataOffset()
    {
        return MEMBER_OFFSET(JSThread, glueData_);
    }

    uintptr_t GetGlueAddr() const
    {
        return reinterpret_cast<uintptr_t>(this) + GetGlueDataOffset();
    }

    static JSThread *GlueToJSThread(uintptr_t glue)
    {
        // very careful to modify here
        return reinterpret_cast<JSThread *>(glue - GetGlueDataOffset());
    }

    bool HasCheckSafePoint() const
    {
        return CheckSafePointBit::Decode(glueData_.interruptVector_);
    }

    void SetVMNeedSuspension(bool flag)
    {
        VMNeedSuspensionBit::Set(flag, &glueData_.interruptVector_);
    }

    bool VMNeedSuspension()
    {
        return VMNeedSuspensionBit::Decode(glueData_.interruptVector_);
    }

    void SetVMSuspended(bool flag)
    {
        VMHasSuspendedBit::Set(flag, &glueData_.interruptVector_);
    }

    bool IsVMSuspended()
    {
        return VMHasSuspendedBit::Decode(glueData_.interruptVector_);
    }

    static uintptr_t GetCurrentStackPosition()
    {
        return reinterpret_cast<uintptr_t>(__builtin_frame_address(0));
    }

    bool IsLegalAsmSp(uintptr_t sp) const;

    bool IsLegalThreadSp(uintptr_t sp) const;

    bool IsLegalSp(uintptr_t sp) const;

    void SetCheckAndCallEnterState(bool state)
    {
        finalizationCheckState_ = state;
    }

    bool GetCheckAndCallEnterState() const
    {
        return finalizationCheckState_;
    }

    uint64_t GetStackStart() const
    {
        return glueData_.stackStart_;
    }

    uint64_t GetStackLimit() const
    {
        return glueData_.stackLimit_;
    }

    void SetStackLimit(uint64_t stackLimit)
    {
        glueData_.stackLimit_ = stackLimit;
    }

    GlobalEnv *GetGlueGlobalEnv()
    {
        return glueData_.glueGlobalEnv_;
    }

    void SetGlueGlobalEnv(GlobalEnv *global)
    {
        ASSERT(global != nullptr);
        glueData_.glueGlobalEnv_ = global;
    }

    inline uintptr_t NewGlobalHandle(JSTaggedType value)
    {
        return newGlobalHandle_(value);
    }

    inline void DisposeGlobalHandle(uintptr_t nodeAddr)
    {
        disposeGlobalHandle_(nodeAddr);
    }

    inline uintptr_t SetWeak(uintptr_t nodeAddr, void *ref = nullptr, WeakClearCallback freeGlobalCallBack = nullptr,
                             WeakClearCallback nativeFinalizeCallBack = nullptr)
    {
        return setWeak_(nodeAddr, ref, freeGlobalCallBack, nativeFinalizeCallBack);
    }

    inline uintptr_t ClearWeak(uintptr_t nodeAddr)
    {
        return clearWeak_(nodeAddr);
    }

    inline bool IsWeak(uintptr_t addr) const
    {
        return isWeak_(addr);
    }

    void EnableCrossThreadExecution()
    {
        glueData_.allowCrossThreadExecution_ = true;
    }

    bool IsCrossThreadExecutionEnable() const
    {
        return glueData_.allowCrossThreadExecution_;
    }

    bool IsFrameDropped()
    {
        return glueData_.isFrameDropped_;
    }

    void SetFrameDroppedState()
    {
        glueData_.isFrameDropped_ = true;
    }

    void ResetFrameDroppedState()
    {
        glueData_.isFrameDropped_ = false;
    }

    bool IsEntryFrameDroppedTrue()
    {
        return glueData_.entryFrameDroppedState_ == FrameDroppedState::StateTrue;
    }

    bool IsEntryFrameDroppedPending()
    {
        return glueData_.entryFrameDroppedState_ == FrameDroppedState::StatePending;
    }

    void SetEntryFrameDroppedState()
    {
        glueData_.entryFrameDroppedState_ = FrameDroppedState::StateTrue;
    }

    void ResetEntryFrameDroppedState()
    {
        glueData_.entryFrameDroppedState_ = FrameDroppedState::StateFalse;
    }

    void PendingEntryFrameDroppedState()
    {
        glueData_.entryFrameDroppedState_ = FrameDroppedState::StatePending;
    }

    bool IsDebugMode()
    {
        return glueData_.isDebugMode_;
    }

    void SetDebugModeState()
    {
        glueData_.isDebugMode_ = true;
    }

    void ResetDebugModeState()
    {
        glueData_.isDebugMode_ = false;
    }

    bool IsStartGlobalLeakCheck() const;
    bool EnableGlobalObjectLeakCheck() const;
    bool EnableGlobalPrimitiveLeakCheck() const;
    void WriteToStackTraceFd(std::ostringstream &buffer) const;
    void SetStackTraceFd(int32_t fd);
    void CloseStackTraceFd();
    uint32_t IncreaseGlobalNumberCount()
    {
        return ++globalNumberCount_;
    }

    struct GlueData : public base::AlignedStruct<JSTaggedValue::TaggedTypeSize(),
                                                 BCStubEntries,
                                                 JSTaggedValue,
                                                 JSTaggedValue,
                                                 base::AlignedBool,
                                                 base::AlignedPointer,
                                                 base::AlignedPointer,
                                                 base::AlignedPointer,
                                                 base::AlignedPointer,
                                                 base::AlignedPointer,
                                                 RTStubEntries,
                                                 COStubEntries,
                                                 BuiltinStubEntries,
                                                 BCDebuggerStubEntries,
                                                 base::AlignedUint64,
                                                 base::AlignedPointer,
                                                 base::AlignedUint64,
                                                 base::AlignedUint64,
                                                 base::AlignedPointer,
                                                 base::AlignedPointer,
                                                 base::AlignedUint64,
                                                 base::AlignedUint64,
                                                 JSTaggedValue,
                                                 base::AlignedBool,
                                                 base::AlignedBool,
                                                 JSTaggedValue,
                                                 base::AlignedPointer> {
        enum class Index : size_t {
            BCStubEntriesIndex = 0,
            ExceptionIndex,
            GlobalObjIndex,
            StableArrayElementsGuardiansIndex,
            CurrentFrameIndex,
            LeaveFrameIndex,
            LastFpIndex,
            NewSpaceAllocationTopAddressIndex,
            NewSpaceAllocationEndAddressIndex,
            RTStubEntriesIndex,
            COStubEntriesIndex,
            BuiltinsStubEntriesIndex,
            BCDebuggerStubEntriesIndex,
            StateBitFieldIndex,
            FrameBaseIndex,
            StackStartIndex,
            StackLimitIndex,
            GlueGlobalEnvIndex,
            GlobalConstIndex,
            AllowCrossThreadExecutionIndex,
            InterruptVectorIndex,
            IsStartHeapSamplingIndex,
            IsDebugModeIndex,
            IsFrameDroppedIndex,
            EntryFrameDroppedStateIndex,
            CurrentContextIndex,
            NumOfMembers
        };
        static_assert(static_cast<size_t>(Index::NumOfMembers) == NumOfTypes);

        static size_t GetExceptionOffset(bool isArch32)
        {
            return GetOffset<static_cast<size_t>(Index::ExceptionIndex)>(isArch32);
        }

        static size_t GetGlobalObjOffset(bool isArch32)
        {
            return GetOffset<static_cast<size_t>(Index::GlobalObjIndex)>(isArch32);
        }

        static size_t GetStableArrayElementsGuardiansOffset(bool isArch32)
        {
            return GetOffset<static_cast<size_t>(Index::StableArrayElementsGuardiansIndex)>(isArch32);
        }

        static size_t GetGlobalConstOffset(bool isArch32)
        {
            return GetOffset<static_cast<size_t>(Index::GlobalConstIndex)>(isArch32);
        }

        static size_t GetStateBitFieldOffset(bool isArch32)
        {
            return GetOffset<static_cast<size_t>(Index::StateBitFieldIndex)>(isArch32);
        }

        static size_t GetCurrentFrameOffset(bool isArch32)
        {
            return GetOffset<static_cast<size_t>(Index::CurrentFrameIndex)>(isArch32);
        }

        static size_t GetLeaveFrameOffset(bool isArch32)
        {
            return GetOffset<static_cast<size_t>(Index::LeaveFrameIndex)>(isArch32);
        }

        static size_t GetLastFpOffset(bool isArch32)
        {
            return GetOffset<static_cast<size_t>(Index::LastFpIndex)>(isArch32);
        }

        static size_t GetNewSpaceAllocationTopAddressOffset(bool isArch32)
        {
            return GetOffset<static_cast<size_t>(Index::NewSpaceAllocationTopAddressIndex)>(isArch32);
        }

        static size_t GetNewSpaceAllocationEndAddressOffset(bool isArch32)
        {
            return GetOffset<static_cast<size_t>(Index::NewSpaceAllocationEndAddressIndex)>(isArch32);
        }

        static size_t GetBCStubEntriesOffset(bool isArch32)
        {
            return GetOffset<static_cast<size_t>(Index::BCStubEntriesIndex)>(isArch32);
        }

        static size_t GetRTStubEntriesOffset(bool isArch32)
        {
            return GetOffset<static_cast<size_t>(Index::RTStubEntriesIndex)>(isArch32);
        }

        static size_t GetCOStubEntriesOffset(bool isArch32)
        {
            return GetOffset<static_cast<size_t>(Index::COStubEntriesIndex)>(isArch32);
        }

        static size_t GetBuiltinsStubEntriesOffset(bool isArch32)
        {
            return GetOffset<static_cast<size_t>(Index::BuiltinsStubEntriesIndex)>(isArch32);
        }

        static size_t GetBCDebuggerStubEntriesOffset(bool isArch32)
        {
            return GetOffset<static_cast<size_t>(Index::BCDebuggerStubEntriesIndex)>(isArch32);
        }

        static size_t GetFrameBaseOffset(bool isArch32)
        {
            return GetOffset<static_cast<size_t>(Index::FrameBaseIndex)>(isArch32);
        }

        static size_t GetStackLimitOffset(bool isArch32)
        {
            return GetOffset<static_cast<size_t>(Index::StackLimitIndex)>(isArch32);
        }

        static size_t GetGlueGlobalEnvOffset(bool isArch32)
        {
            return GetOffset<static_cast<size_t>(Index::GlueGlobalEnvIndex)>(isArch32);
        }

        static size_t GetAllowCrossThreadExecutionOffset(bool isArch32)
        {
            return GetOffset<static_cast<size_t>(Index::AllowCrossThreadExecutionIndex)>(isArch32);
        }

        static size_t GetInterruptVectorOffset(bool isArch32)
        {
            return GetOffset<static_cast<size_t>(Index::InterruptVectorIndex)>(isArch32);
        }

        static size_t GetIsStartHeapSamplingOffset(bool isArch32)
        {
            return GetOffset<static_cast<size_t>(Index::IsStartHeapSamplingIndex)>(isArch32);
        }

        static size_t GetIsDebugModeOffset(bool isArch32)
        {
            return GetOffset<static_cast<size_t>(Index::IsDebugModeIndex)>(isArch32);
        }

        static size_t GetIsFrameDroppedOffset(bool isArch32)
        {
            return GetOffset<static_cast<size_t>(Index::IsFrameDroppedIndex)>(isArch32);
        }

        static size_t GetEntryFrameDroppedStateOffset(bool isArch32)
        {
            return GetOffset<static_cast<size_t>(Index::EntryFrameDroppedStateIndex)>(isArch32);
        }

        static size_t GetCurrentContextOffset(bool isArch32)
        {
            return GetOffset<static_cast<size_t>(Index::CurrentContextIndex)>(isArch32);
        }

        alignas(EAS) BCStubEntries bcStubEntries_;
        alignas(EAS) JSTaggedValue exception_ {JSTaggedValue::Hole()};
        alignas(EAS) JSTaggedValue globalObject_ {JSTaggedValue::Hole()};
        alignas(EAS) bool stableArrayElementsGuardians_ {true};
        alignas(EAS) JSTaggedType *currentFrame_ {nullptr};
        alignas(EAS) JSTaggedType *leaveFrame_ {nullptr};
        alignas(EAS) JSTaggedType *lastFp_ {nullptr};
        alignas(EAS) const uintptr_t *newSpaceAllocationTopAddress_ {nullptr};
        alignas(EAS) const uintptr_t *newSpaceAllocationEndAddress_ {nullptr};
        alignas(EAS) RTStubEntries rtStubEntries_;
        alignas(EAS) COStubEntries coStubEntries_;
        alignas(EAS) BuiltinStubEntries builtinStubEntries_;
        alignas(EAS) BCDebuggerStubEntries bcDebuggerStubEntries_;
        alignas(EAS) volatile uint64_t gcStateBitField_ {0ULL};
        alignas(EAS) JSTaggedType *frameBase_ {nullptr};
        alignas(EAS) uint64_t stackStart_ {0};
        alignas(EAS) uint64_t stackLimit_ {0};
        alignas(EAS) GlobalEnv *glueGlobalEnv_;
        alignas(EAS) GlobalEnvConstants *globalConst_;
        alignas(EAS) bool allowCrossThreadExecution_ {false};
        alignas(EAS) volatile uint64_t interruptVector_ {0};
        alignas(EAS) JSTaggedValue isStartHeapSampling_ {JSTaggedValue::False()};
        alignas(EAS) bool isDebugMode_ {false};
        alignas(EAS) bool isFrameDropped_ {false};
        alignas(EAS) uint64_t entryFrameDroppedState_ {FrameDroppedState::StateFalse};
        alignas(EAS) EcmaContext *currentContext_ {nullptr};
    };
    STATIC_ASSERT_EQ_ARCH(sizeof(GlueData), GlueData::SizeArch32, GlueData::SizeArch64);

    void PushContext(EcmaContext *context);
    void PopContext();

    EcmaContext *GetCurrentEcmaContext() const
    {
        return glueData_.currentContext_;
    }
    void SwitchCurrentContext(EcmaContext *currentContext, bool isInIterate = false);

    CVector<EcmaContext *> GetEcmaContexts()
    {
        return contexts_;
    }

    bool EraseContext(EcmaContext *context);

    const GlobalEnvConstants *GetFirstGlobalConst() const;
    bool IsAllContextsInitialized() const;
    void ResetCheckSafePointStatus();
    void SetCheckSafePointStatus();
private:
    NO_COPY_SEMANTIC(JSThread);
    NO_MOVE_SEMANTIC(JSThread);
    void SetGlobalConst(GlobalEnvConstants *globalConst)
    {
        glueData_.globalConst_ = globalConst;
    }
    void SetCurrentEcmaContext(EcmaContext *context)
    {
        glueData_.currentContext_ = context;
    }

    void SetArrayHClassIndexMap(const CMap<ElementsKind, ConstantIndex> &map)
    {
        arrayHClassIndexMap_ = map;
    }

    void DumpStack() DUMP_API_ATTR;

    static size_t GetAsmStackLimit();

    static bool IsMainThread();

    static constexpr size_t DEFAULT_MAX_SYSTEM_STACK_SIZE = 8_MB;

    GlueData glueData_;
    std::atomic<ThreadId> id_;
    EcmaVM *vm_ {nullptr};

    // MM: handles, global-handles, and aot-stubs.
    int nestedLevel_ = 0;
    NativeAreaAllocator *nativeAreaAllocator_ {nullptr};
    HeapRegionAllocator *heapRegionAllocator_ {nullptr};
    std::vector<std::pair<WeakClearCallback, void *>> weakNodeNativeFinalizeCallbacks_ {};

    EcmaGlobalStorage<Node> *globalStorage_ {nullptr};
    EcmaGlobalStorage<DebugNode> *globalDebugStorage_ {nullptr};
    int32_t stackTraceFd_ {-1};

    std::function<uintptr_t(JSTaggedType value)> newGlobalHandle_;
    std::function<void(uintptr_t nodeAddr)> disposeGlobalHandle_;
    std::function<uintptr_t(uintptr_t nodeAddr, void *ref, WeakClearCallback freeGlobalCallBack_,
         WeakClearCallback nativeFinalizeCallBack)> setWeak_;
    std::function<uintptr_t(uintptr_t nodeAddr)> clearWeak_;
    std::function<bool(uintptr_t addr)> isWeak_;
    uint32_t globalNumberCount_ {0};

    // Run-time state
    bool getStackSignal_ {false};
    bool runtimeState_ {false};
    bool isAsmInterpreter_ {false};
    VmThreadControl *vmThreadControl_ {nullptr};

    // CpuProfiler
    bool isProfiling_ {false};
    bool gcState_ {false};
    std::atomic_bool needProfiling_ {false};
    std::string profileName_ {""};

    bool finalizationCheckState_ {false};

    CMap<ElementsKind, ConstantIndex> arrayHClassIndexMap_;

    CVector<EcmaContext *> contexts_;
    friend class GlobalHandleCollection;
    friend class EcmaVM;
    friend class EcmaContext;
};
}  // namespace panda::ecmascript
#endif  // ECMASCRIPT_JS_THREAD_H
