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

#ifndef ECMASCRIPT_FRAMES_H
#define ECMASCRIPT_FRAMES_H

#include "ecmascript/base/aligned_struct.h"
#include "ecmascript/js_tagged_value.h"
#include "ecmascript/mem/visitor.h"
#include "ecmascript/method.h"
#include "ecmascript/stackmap/ark_stackmap.h"
#include "ecmascript/stackmap/llvm/llvm_stackmap_type.h"

namespace panda::ecmascript {
class JSThread;
class EcmaVM;
class FrameIterator;
namespace kungfu {
    class ArkStackMapParser;
};

static constexpr int64_t BASELINEJIT_PC_FLAG = static_cast<int64_t>(std::numeric_limits<uint64_t>::max());

// Here list all scenarios of calling between Runtime/CInterpreter/ASMInterpreter/AOTCompiler/CBuiltin/ASMBuitlin.
// Please note that the "[]" means a must frame while "<>" means an optional frame. Each case is from top to down.
//
// * Runtime (C++) => CInterpreter:
//          1) [INTERPRETER_FRAME]
//
// * Runtime (C++) -> AOTCompiler:
//          1) [OPTIMIZED_ENTRY_FRAME]
//             <OPTIMIZED_JS_FUNCTION_UNFOLD_ARGV_FRAME>
//             <OPTIMIZED_JS_FUNCTION_ARGS_CONFIG_FRAME>
//             [OPTIMIZED_JS_FUNCTION_FRAME]
//
// * Runtime (C++) => ASMInterpreter:
//          1) [INTERPRETER_ENTRY_FRAME][ASM_INTERPRETER_FRAME]
//
// * Runtime (C++) => CBuiltin:
//          1) [not supported]
//
// * Runtime (C++) => ASMBuiltin:
//          1) [not supported]
//
// * CInterpreter => CInterpreter:
//          1) [INTERPRETER_FRAME]
//
// * CInterpreter => Runtime (C++):
//          1) [INTERPRETER_FAST_NEW_FRAME]
//          2) [INTERPRETER_CONSTRUCTOR_FRAME]
//
// * CInterpreter => AOTCompiler:
//          1) [not supported]
//
// * CInterperter => CBuiltin:
//          1) [INTERPRETER_BUILTIN_FRAME]
//
// * CInterpreter => ASMBuiltin:
//          1) [not supported]
//
// * ASMInterpreter => Runtime (C++):
//          1) [LEAVE_FRAME]
//          2) [LEAVE_FRAME_WITH_ARGV]
//
// * ASMInterpreter => AOTCompiler:
//          1) [OPTIMIZED_ENTRY_FRAME]
//             <OPTIMIZED_JS_FUNCTION_UNFOLD_ARGV_FRAME>
//             <OPTIMIZED_JS_FUNCTION_ARGS_CONFIG_FRAME>
//             [OPTIMIZED_JS_FUNCTION_FRAME]
//
// * ASMInterpreter => ASMInterpreter:
//          1) [ASM_INTERPRETER_FRAME]
//
// * ASMInterpreter => AsmBuiltin:
//          1) [BUILTIN_ENTRY_FRAME]
//             [BUILTIN_FRAME]
//          2) [BUILTIN_ENTRY_FRAME]
//             [BUILTIN_FRAME_WITH_ARGV]
//
// * ASMInterpreter => CBuiltin:
//          1) [LEAVE_FRAME]
//          2) [LEAVE_FRAME_WITH_ARGV]
//
// * AOTCompiler => Runtime (C++):
//          1) [LEAVE_FRAME]
//          2) [LEAVE_FRAME_WITH_ARGV]
//
// * AOTCompiler => ASMInterpreter:
//          1) [ASM_INTERPRETER_BRIDGE_FRAME]
//          2) [ASM_INTERPRETER_FRAME]
//
// * AOTCompiler => CBuiltin:
//          1) [LEAVE_FRAME]
//          2) [LEAVE_FRAME_WITH_ARGV]
//
// * AOTCompiler => ASMBuiltin:
//          1) [BUILTIN_ENTRY_FRAME]
//             [BUILTIN_FRAME]
//          2) [BUILTIN_ENTRY_FRAME]
//             [BUILTIN_FRAME_WITH_ARGV]


enum class FrameType: uintptr_t {
    OPTIMIZED_FRAME = 0,
    OPTIMIZED_ENTRY_FRAME,
    OPTIMIZED_JS_FUNCTION_FRAME,
    OPTIMIZED_JS_FAST_CALL_FUNCTION_FRAME,
    FASTJIT_FUNCTION_FRAME,
    FASTJIT_FAST_CALL_FUNCTION_FRAME,
    ASM_BRIDGE_FRAME,
    LEAVE_FRAME,
    LEAVE_FRAME_WITH_ARGV,
    BUILTIN_CALL_LEAVE_FRAME,
    INTERPRETER_FRAME,
    ASM_INTERPRETER_FRAME,
    INTERPRETER_CONSTRUCTOR_FRAME,
    BUILTIN_FRAME,
    BUILTIN_FRAME_WITH_ARGV,
    BUILTIN_ENTRY_FRAME,
    INTERPRETER_BUILTIN_FRAME,
    INTERPRETER_FAST_NEW_FRAME,
    INTERPRETER_ENTRY_FRAME,
    ASM_INTERPRETER_ENTRY_FRAME,
    ASM_INTERPRETER_BRIDGE_FRAME,
    OPTIMIZED_JS_FUNCTION_ARGS_CONFIG_FRAME,
    OPTIMIZED_JS_FUNCTION_UNFOLD_ARGV_FRAME,
    BUILTIN_FRAME_WITH_ARGV_STACK_OVER_FLOW_FRAME,
    BASELINE_BUILTIN_FRAME,

    FRAME_TYPE_FIRST = OPTIMIZED_FRAME,
    FRAME_TYPE_LAST = OPTIMIZED_JS_FUNCTION_UNFOLD_ARGV_FRAME,
    INTERPRETER_FIRST = INTERPRETER_FRAME,
    INTERPRETER_LAST = INTERPRETER_FAST_NEW_FRAME,
    BUILTIN_FIRST = BUILTIN_FRAME,
    BUILTIN_LAST = BUILTIN_ENTRY_FRAME,
};

enum class JSCallMode : uintptr_t {
    CALL_ARG0 = 0,
    CALL_ARG1,
    CALL_ARG2,
    CALL_ARG3,
    CALL_THIS_ARG0,
    CALL_THIS_ARG1,
    CALL_THIS_ARG2,
    CALL_THIS_ARG3,
    CALL_WITH_ARGV,
    CALL_THIS_WITH_ARGV,
    CALL_CONSTRUCTOR_WITH_ARGV,
    SUPER_CALL_WITH_ARGV,
    SUPER_CALL_SPREAD_WITH_ARGV,
    DEPRECATED_CALL_ARG0,
    DEPRECATED_CALL_ARG1,
    DEPRECATED_CALL_ARG2,
    DEPRECATED_CALL_ARG3,
    DEPRECATED_CALL_WITH_ARGV,
    DEPRECATED_CALL_THIS_WITH_ARGV,
    DEPRECATED_CALL_CONSTRUCTOR_WITH_ARGV,
    CALL_GETTER,
    CALL_SETTER,
    CALL_THIS_ARG2_WITH_RETURN,
    CALL_THIS_ARG3_WITH_RETURN,
    CALL_THIS_ARGV_WITH_RETURN,
    CALL_ENTRY,
    CALL_GENERATOR,
    CALL_FROM_AOT,
};

// NOLINTNEXTLINE(cppcoreguidelines-pro-type-member-init)
struct OptimizedFrame : public base::AlignedStruct<base::AlignedPointer::Size(),
                                                   base::AlignedPointer,
                                                   base::AlignedPointer,
                                                   base::AlignedPointer> {
public:
    void GCIterate(const FrameIterator &it, RootVisitor &visitor) const;

    static size_t GetTypeOffset(bool isArch32 = false)
    {
        return GetOffset<static_cast<size_t>(Index::TypeIndex)>(isArch32);
    }

    static size_t GetPrevOffset(bool isArch32 = false)
    {
        return GetOffset<static_cast<size_t>(Index::PrevFpIndex)>(isArch32);
    }

    static size_t GetReturnAddrOffset(bool isArch32 = false)
    {
        return GetOffset<static_cast<size_t>(Index::ReturnAddrIndex)>(isArch32);
    }

    static size_t ComputeReservedSize(size_t slotSize)
    {
        size_t slotOffset = static_cast<size_t>(Index::PrevFpIndex) - static_cast<size_t>(Index::TypeIndex);
        return slotSize * slotOffset;
    }

    FrameType GetType() const
    {
        return type;
    }

private:
    enum class Index : size_t {
        TypeIndex = 0,
        PrevFpIndex,
        ReturnAddrIndex,
        NumOfMembers
    };
    static_assert(static_cast<size_t>(Index::NumOfMembers) == NumOfTypes);

    static OptimizedFrame* GetFrameFromSp(const JSTaggedType *sp)
    {
        return reinterpret_cast<OptimizedFrame *>(reinterpret_cast<uintptr_t>(sp) -
            MEMBER_OFFSET(OptimizedFrame, prevFp));
    }
    inline JSTaggedType* GetPrevFrameFp()
    {
        return prevFp;
    }
    uintptr_t GetReturnAddr() const
    {
        return returnAddr;
    }

    alignas(EAS) FrameType type {0};
    alignas(EAS) JSTaggedType *prevFp {nullptr};
    alignas(EAS) uintptr_t returnAddr {0};
    friend class FrameIterator;
};
STATIC_ASSERT_EQ_ARCH(sizeof(OptimizedFrame), OptimizedFrame::SizeArch32, OptimizedFrame::SizeArch64);

// * BaselineBuiltinFrame layout as the following:
//               +--------------------------+ ---------
//               |       . . . . .          |         ^
// callerSP ---> |--------------------------|         |
//               |       returnAddr         |         |
//               |--------------------------|   BuiltinBuiltinFrame
//               |       callsiteFp         |         |
//       fp ---> |--------------------------|         |
//               |       frameType          |         v
//               +--------------------------+ ---------
//               |        . . . .           |
// calleeSP ---> +--------------------------+
//
// NOLINTNEXTLINE(cppcoreguidelines-pro-type-member-init)
struct BaselineBuiltinFrame : public base::AlignedStruct<base::AlignedPointer::Size(),
                                                         base::AlignedPointer,
                                                         base::AlignedPointer,
                                                         base::AlignedPointer> {
public:
    void GCIterate(const FrameIterator &it, RootVisitor &visitor) const;

    static size_t GetTypeOffset(bool isArch32 = false)
    {
        return GetOffset<static_cast<size_t>(Index::TypeIndex)>(isArch32);
    }

    static size_t GetPrevOffset(bool isArch32 = false)
    {
        return GetOffset<static_cast<size_t>(Index::PrevFpIndex)>(isArch32);
    }

    static size_t GetReturnAddrOffset(bool isArch32 = false)
    {
        return GetOffset<static_cast<size_t>(Index::ReturnAddrIndex)>(isArch32);
    }

    static size_t ComputeReservedSize(size_t slotSize)
    {
        size_t slotOffset = static_cast<size_t>(Index::PrevFpIndex) - static_cast<size_t>(Index::TypeIndex);
        return slotSize * slotOffset;
    }

    FrameType GetType() const
    {
        return type;
    }

    uintptr_t GetReturnAddr() const
    {
        return returnAddr;
    }

private:
    enum class Index : size_t {
        TypeIndex = 0,
        PrevFpIndex,
        ReturnAddrIndex,
        NumOfMembers
    };
    static_assert(static_cast<size_t>(Index::NumOfMembers) == NumOfTypes);

    static BaselineBuiltinFrame* GetFrameFromSp(const JSTaggedType *sp)
    {
        return reinterpret_cast<BaselineBuiltinFrame *>(reinterpret_cast<uintptr_t>(sp) -
            MEMBER_OFFSET(BaselineBuiltinFrame, prevFp));
    }
    inline JSTaggedType* GetPrevFrameFp()
    {
        return prevFp;
    }

    alignas(EAS) FrameType type {0};
    alignas(EAS) JSTaggedType *prevFp {nullptr};
    alignas(EAS) uintptr_t returnAddr {0};
    friend class FrameIterator;
};
STATIC_ASSERT_EQ_ARCH(sizeof(BaselineBuiltinFrame), BaselineBuiltinFrame::SizeArch32, BaselineBuiltinFrame::SizeArch64);

struct AsmBridgeFrame : public base::AlignedStruct<base::AlignedPointer::Size(),
                                                   base::AlignedPointer,
                                                   base::AlignedPointer,
                                                   base::AlignedPointer> {
public:
    static size_t GetTypeOffset(bool isArch32 = false)
    {
        return GetOffset<static_cast<size_t>(Index::TypeIndex)>(isArch32);
    }

    static size_t GetPrevOffset(bool isArch32 = false)
    {
        return GetOffset<static_cast<size_t>(Index::PrevFpIndex)>(isArch32);
    }

    static size_t GetReturnAddrOffset(bool isArch32 = false)
    {
        return GetOffset<static_cast<size_t>(Index::ReturnAddrIndex)>(isArch32);
    }

    uintptr_t GetCallSiteSp() const
    {
        return ToUintPtr(this) + sizeof(AsmBridgeFrame);
    }

    FrameType GetType() const
    {
        return type;
    }

private:
    enum class Index : size_t {
        TypeIndex = 0,
        PrevFpIndex,
        ReturnAddrIndex,
        NumOfMembers
    };
    static_assert(static_cast<size_t>(Index::NumOfMembers) == NumOfTypes);

    static AsmBridgeFrame* GetFrameFromSp(const JSTaggedType *sp)
    {
        return reinterpret_cast<AsmBridgeFrame *>(reinterpret_cast<uintptr_t>(sp) -
            MEMBER_OFFSET(AsmBridgeFrame, prevFp));
    }
    inline JSTaggedType* GetPrevFrameFp()
    {
        return prevFp;
    }
    uintptr_t GetReturnAddr() const
    {
        return returnAddr;
    }
    alignas(EAS) FrameType type {0};
    alignas(EAS) JSTaggedType *prevFp {nullptr};
    alignas(EAS) uintptr_t returnAddr {0};
    friend class FrameIterator;
};
STATIC_ASSERT_EQ_ARCH(sizeof(AsmBridgeFrame), AsmBridgeFrame::SizeArch32, AsmBridgeFrame::SizeArch64);

// * OptimizedUnfoldArgVFrame layout description as the following:
// callerSP ---> |--------------------------| ---------------
//               |       returnAddr         |               ^
//               |--------------------------|               |
//               |       prevFp             |               |
//       fp ---> |--------------------------|   OptimizedUnfoldArgVFrame
//               |       frameType          |               |
//               |--------------------------|               |
//               |       currentFp          |               v
// calleESP ---> +--------------------------+ ---------------
//
// NOLINTNEXTLINE(cppcoreguidelines-pro-type-member-init)
struct OptimizedJSFunctionUnfoldArgVFrame : public base::AlignedStruct<base::AlignedPointer::Size(),
                                                                       base::AlignedPointer,
                                                                       base::AlignedPointer,
                                                                       base::AlignedPointer,
                                                                       base::AlignedPointer> {
public:
    static size_t GetTypeOffset(bool isArch32 = false)
    {
        return GetOffset<static_cast<size_t>(Index::TypeIndex)>(isArch32);
    }

    static size_t GetPrevOffset(bool isArch32 = false)
    {
        return GetOffset<static_cast<size_t>(Index::PrevFpIndex)>(isArch32);
    }

    static size_t GetReturnAddrOffset(bool isArch32 = false)
    {
        return GetOffset<static_cast<size_t>(Index::ReturnAddrIndex)>(isArch32);
    }

    FrameType GetType() const
    {
        return type;
    }

private:
    enum class Index : size_t {
        CallSiteSpIndex = 0,
        TypeIndex,
        PrevFpIndex,
        ReturnAddrIndex,
        NumOfMembers
    };
    static_assert(static_cast<size_t>(Index::NumOfMembers) == NumOfTypes);

    static OptimizedJSFunctionUnfoldArgVFrame* GetFrameFromSp(const JSTaggedType *sp)
    {
        return reinterpret_cast<OptimizedJSFunctionUnfoldArgVFrame *>(reinterpret_cast<uintptr_t>(sp) -
            MEMBER_OFFSET(OptimizedJSFunctionUnfoldArgVFrame, prevFp));
    }
    inline JSTaggedType* GetPrevFrameFp() const
    {
        return prevFp;
    }
    uintptr_t GetReturnAddr() const
    {
        return returnAddr;
    }
    uintptr_t GetPrevFrameSp() const
    {
        return callSiteSp;
    }
    alignas(EAS) uintptr_t callSiteSp {0};
    alignas(EAS) FrameType type {0};
    alignas(EAS) JSTaggedType *prevFp {nullptr};
    alignas(EAS) uintptr_t returnAddr {0};
    friend class FrameIterator;
};
STATIC_ASSERT_EQ_ARCH(sizeof(OptimizedJSFunctionUnfoldArgVFrame),
    OptimizedJSFunctionUnfoldArgVFrame::SizeArch32, OptimizedJSFunctionUnfoldArgVFrame::SizeArch64);

// * The OptimizedJSFunctionArgsConfig Frame's structure is illustrated as the following:
//          +--------------------------+
//          |         arg[N-1]         |
//          +--------------------------+
//          |         . . . .          |
//          +--------------------------+
//          |         arg[0]           |
//          +--------------------------+
//          |         argC             |
//  sp ---> +--------------------------+ -----------------
//          |                          |                 ^
//          |        prevFP            |                 |
//  fp ---> |--------------------------|    OptimizedJSFunctionArgsConfigFrame
//          |       frameType          |                 |
//          |                          |                 V
//          +--------------------------+ -----------------
//
// NOLINTNEXTLINE(cppcoreguidelines-pro-type-member-init)
struct OptimizedJSFunctionArgConfigFrame : public base::AlignedStruct<base::AlignedPointer::Size(),
                                                                      base::AlignedPointer,
                                                                      base::AlignedPointer> {
public:
    static size_t GetTypeOffset(bool isArch32 = false)
    {
        return GetOffset<static_cast<size_t>(Index::TypeIndex)>(isArch32);
    }

    static size_t GetPrevOffset(bool isArch32 = false)
    {
        return GetOffset<static_cast<size_t>(Index::PrevFpIndex)>(isArch32);
    }

    FrameType GetType() const
    {
        return type;
    }

private:
    enum class Index : size_t {
        TypeIndex = 0,
        PrevFpIndex,
        NumOfMembers
    };
    static_assert(static_cast<size_t>(Index::NumOfMembers) == NumOfTypes);

    static OptimizedJSFunctionArgConfigFrame* GetFrameFromSp(const JSTaggedType *sp)
    {
        return reinterpret_cast<OptimizedJSFunctionArgConfigFrame *>(reinterpret_cast<uintptr_t>(sp) -
            MEMBER_OFFSET(OptimizedJSFunctionArgConfigFrame, prevFp));
    }
    inline JSTaggedType* GetPrevFrameFp()
    {
        return prevFp;
    }

    alignas(EAS) FrameType type {0};
    alignas(EAS) JSTaggedType *prevFp {nullptr};
    friend class FrameIterator;
};
STATIC_ASSERT_EQ_ARCH(sizeof(OptimizedJSFunctionArgConfigFrame),
                      OptimizedJSFunctionArgConfigFrame::SizeArch32,
                      OptimizedJSFunctionArgConfigFrame::SizeArch64);

// * OptimizedJSFunctionFrame layout description as the following:
//                 +----------------------------------------+
//                 |     arg[N-1]                           |
//                 +----------------------------------------+
//                 |     ...                                |
//                 +----------------------------------------+
//                 |     arg[1]                             |
//                 +----------------------------------------+
//                 |     arg[0]                             |
//                 +----------------------------------------+
//                 |     this                               |
//                 +----------------------------------------+
//                 |     new-target [not exist in fastcall] |
//                 +----------------------------------------+
//                 |     call-target                        |
//       argv ---> +----------------------------------------+
//                 |     argv   [not exist in fastcall]     |
//                 |----------------------------------------|
//                 |     argc   [not exist in fastcall]     |
//   callerSp ---> |----------------------------------------|----------------
//                 |     returnAddr                         |               ^
//                 |----------------------------------------|               |
//                 |     callsiteFp                         |               |
//         fp ---> |----------------------------------------|   OptimizedJSFunctionFrame
//                 |     frameType                          |               |
//                 |----------------------------------------|               |
//                 |     call-target                        |               v
//   calleeSP ---> +----------------------------------------+----------------
//
// NOLINTNEXTLINE(cppcoreguidelines-pro-type-member-init)
struct OptimizedJSFunctionFrame : public base::AlignedStruct<JSTaggedValue::TaggedTypeSize(),
                                                             JSTaggedValue,
                                                             base::AlignedPointer,
                                                             base::AlignedPointer,
                                                             base::AlignedPointer> {
public:
    using ConstInfo = kungfu::LLVMStackMapType::ConstInfo;
    enum class Index : size_t {
        JSFuncIndex = 0,
        TypeIndex,
        PrevFpIndex,
        ReturnAddrIndex,
        NumOfMembers
    };
    static_assert(static_cast<size_t>(Index::NumOfMembers) == NumOfTypes);

    static constexpr size_t GetFunctionDeltaReturnAddr()
    {
        return static_cast<size_t>(Index::ReturnAddrIndex) - static_cast<size_t>(Index::JSFuncIndex);
    }

    inline JSTaggedType* GetPrevFrameFp()
    {
        return prevFp;
    }

    JSTaggedType* GetArgv(uintptr_t *preFrameSp) const
    {
        const size_t offset = 2;    // 2: skip argc and argv.
        return reinterpret_cast<JSTaggedType *>(preFrameSp + offset * sizeof(uint64_t) / sizeof(uintptr_t));
    }

    size_t GetArgc(uintptr_t *preFrameSp) const
    {
        return *preFrameSp;
    }

    JSTaggedType* GetArgv(const FrameIterator &it) const;

    uintptr_t GetReturnAddr() const
    {
        return returnAddr;
    }

    void GCIterate(const FrameIterator &it, RootVisitor &visitor, FrameType frameType) const;
    void CollectPcOffsetInfo(const FrameIterator &it, ConstInfo &info) const;

    inline JSTaggedValue GetFunction() const
    {
        return jsFunc;
    }

    static uintptr_t ComputeArgsConfigFrameSp(JSTaggedType *fp)
    {
        const size_t offset = 2;  // 2: skip prevFp and return address.
        return reinterpret_cast<uintptr_t>(fp) + offset * sizeof(uintptr_t);
    }

    static size_t GetTypeOffset(bool isArch32 = false)
    {
        return GetOffset<static_cast<size_t>(Index::TypeIndex)>(isArch32);
    }

    static size_t GetPrevOffset(bool isArch32 = false)
    {
        return GetOffset<static_cast<size_t>(Index::PrevFpIndex)>(isArch32);
    }

    static size_t GetFunctionOffset(bool isArch32 = false)
    {
        return GetOffset<static_cast<size_t>(Index::JSFuncIndex)>(isArch32);
    }

    static size_t GetReturnAddrOffset(bool isArch32 = false)
    {
        return GetOffset<static_cast<size_t>(Index::ReturnAddrIndex)>(isArch32);
    }

    static size_t ComputeReservedJSFuncOffset(size_t slotSize)
    {
        size_t slotOffset = static_cast<size_t>(Index::PrevFpIndex) - static_cast<size_t>(Index::JSFuncIndex);
        return slotSize * slotOffset;
    }

    static int GetFrmaeTypeToFpDelta()
    {
        return -(int)sizeof(uintptr_t);
    }

    static int GetFunctionToFpDelta()
    {
        int slotOffset = static_cast<int>(Index::JSFuncIndex) - static_cast<int>(Index::TypeIndex);
        return slotOffset * static_cast<int>(JSTaggedValue::TaggedTypeSize()) + GetFrmaeTypeToFpDelta();
    }

    FrameType GetType() const
    {
        return type;
    }

    friend class FrameIterator;
    friend class FrameHandler;
    void GetDeoptBundleInfo(const FrameIterator &it, std::vector<kungfu::ARKDeopt>& deopts) const;
    void GetFuncCalleeRegAndOffset(
        const FrameIterator &it, kungfu::CalleeRegAndOffsetVec &ret) const;
    uintptr_t* ComputePrevFrameSp(const FrameIterator &it) const;

private:
    static OptimizedJSFunctionFrame* GetFrameFromSp(const JSTaggedType *sp)
    {
        return reinterpret_cast<OptimizedJSFunctionFrame *>(reinterpret_cast<uintptr_t>(sp) -
            MEMBER_OFFSET(OptimizedJSFunctionFrame, prevFp));
    }

    // dynamic callee saveregisters for x86-64
    alignas(EAS) JSTaggedValue jsFunc {JSTaggedValue::Undefined()};
    alignas(EAS) FrameType type {0};
    alignas(EAS) JSTaggedType *prevFp {nullptr};
    alignas(EAS) uintptr_t returnAddr {0};
    // dynamic callee saveregisters for arm64
};
STATIC_ASSERT_EQ_ARCH(sizeof(OptimizedJSFunctionFrame),
                      OptimizedJSFunctionFrame::SizeArch32,
                      OptimizedJSFunctionFrame::SizeArch64);
// 2: return addr & prevFp, type and js function should be pairs to update type and js function at the same time.
static_assert((OptimizedJSFunctionFrame::GetFunctionDeltaReturnAddr() % 2) == 1);

// * The JSFunctionEntry Frame's structure is illustrated as the following:
//              +--------------------------+
//              |      . . . . . .         |
// callerSP --> +--------------------------+ -----------------
//              |        prevFP            |                 ^
//       fp --> |--------------------------|                 |
//              |       frameType          |      JSFunctionEntryFrame
//              |--------------------------|                 |
//              |    preLeaveFrameFp       |                 v
// calleeSP --> +--------------------------+ -----------------

struct OptimizedEntryFrame : public base::AlignedStruct<base::AlignedPointer::Size(),
                                                        base::AlignedPointer,
                                                        base::AlignedPointer,
                                                        base::AlignedPointer> {
public:
    enum class CallType : size_t {
        CALL_FUNC = 0,
        CALL_NEW,
    };

    enum class Index : size_t {
        PreLeaveFrameFpIndex = 0,
        TypeIndex,
        PrevFpIndex,
        NumOfMembers
    };

    static size_t GetTypeOffset(bool isArch32 = false)
    {
        return GetOffset<static_cast<size_t>(Index::TypeIndex)>(isArch32);
    }

    static size_t GetLeaveFrameFpOffset(bool isArch32 = false)
    {
        return GetOffset<static_cast<size_t>(Index::PreLeaveFrameFpIndex)>(isArch32);
    }

    inline JSTaggedType* GetPrevFrameFp()
    {
        return preLeaveFrameFp;
    }

    static size_t ComputeReservedSize(size_t slotSize)
    {
        size_t slotOffset = static_cast<size_t>(Index::PrevFpIndex) - static_cast<size_t>(Index::PreLeaveFrameFpIndex);
        return slotSize * slotOffset;
    }

    FrameType GetType() const
    {
        return type;
    }
    friend class FrameIterator;

private:
    static OptimizedEntryFrame* GetFrameFromSp(const JSTaggedType *sp)
    {
        return reinterpret_cast<OptimizedEntryFrame *>(reinterpret_cast<uintptr_t>(sp) -
                                                       MEMBER_OFFSET(OptimizedEntryFrame, prevFp));
    }

    JSTaggedType* GetLeaveFp() const
    {
        return preLeaveFrameFp;
    }

    JSTaggedType* GetPrevFp() const
    {
        return prevFp;
    }

    alignas(EAS) JSTaggedType *preLeaveFrameFp {nullptr};
    alignas(EAS) FrameType type {0};
    alignas(EAS) JSTaggedType *prevFp {nullptr};
};
STATIC_ASSERT_EQ_ARCH(sizeof(OptimizedEntryFrame), OptimizedEntryFrame::SizeArch32, OptimizedEntryFrame::SizeArch64);

// NOLINTNEXTLINE(cppcoreguidelines-pro-type-member-init)
struct InterpretedFrameBase : public base::AlignedStruct<base::AlignedPointer::Size(),
                                                         base::AlignedPointer,
                                                         base::AlignedSize> {
    enum class Index : size_t {
        PrevIndex = 0,
        TypeIndex,
        NumOfMembers
    };
    static_assert(static_cast<size_t>(Index::NumOfMembers) == NumOfTypes);

    inline JSTaggedType* GetPrevFrameFp()
    {
        return prev;
    }

    static InterpretedFrameBase* GetFrameFromSp(const JSTaggedType *sp)
    {
        return reinterpret_cast<InterpretedFrameBase *>(const_cast<JSTaggedType *>(sp)) - 1;
    }

    static size_t GetPrevOffset(bool isArch32 = false)
    {
        return GetOffset<static_cast<size_t>(Index::PrevIndex)>(isArch32);
    }

    static size_t GetTypeOffset(bool isArch32 = false)
    {
        return GetOffset<static_cast<size_t>(Index::TypeIndex)>(isArch32);
    }

    static constexpr size_t GetSize(bool isArch32)
    {
        return isArch32 ? InterpretedFrameBase::SizeArch32 : InterpretedFrameBase::SizeArch64;
    }

    alignas(EAS) JSTaggedType *prev {nullptr}; // for llvm :c-fp ; for interrupt: thread-fp for gc
    alignas(EAS) FrameType type {FrameType::OPTIMIZED_FRAME}; // 0
};
STATIC_ASSERT_EQ_ARCH(sizeof(InterpretedFrameBase),
                      InterpretedFrameBase::SizeArch32,
                      InterpretedFrameBase::SizeArch64);

// Interpreter Frame Layout as the following:
//   +----------------------------------+
//   |    argv[n-1]                     |
//   |----------------------------------|
//   |    ......                        |
//   |----------------------------------|
//   |    thisArg [maybe not exist]     |
//   |----------------------------------|
//   |    newTarget [maybe not exist]   |
//   |----------------------------------|
//   |    ......                        |
//   |----------------------------------|
//   |    Vregs [not exist in native]   |
//   +----------------------------------+--------+
//   |    base.frameType                |        ^
//   |----------------------------------|        |
//   |    base.prev(prev stack pointer) |        |
//   |----------------------------------|        |
//   |    pc(bytecode addr)             |        |
//   |----------------------------------|        |
//   |    sp(current stack pointer)     |        |
//   |----------------------------------|        |
//   |    env                           |        |
//   |----------------------------------|        |
//   |    acc                           |        |
//   |----------------------------------|   InterpretedFrame
//   |    profileTypeInfo               |        |
//   |----------------------------------|        |
//   |    thisObj                       |        |
//   |----------------------------------|        |
//   |    function                      |        |
//   |----------------------------------|        |
//   |    constpool                     |        v
//   +----------------------------------+--------+
//
// NOLINTNEXTLINE(cppcoreguidelines-pro-type-member-init)
struct InterpretedFrame : public base::AlignedStruct<JSTaggedValue::TaggedTypeSize(),
                                                     JSTaggedValue,
                                                     JSTaggedValue,
                                                     JSTaggedValue,
                                                     JSTaggedValue,
                                                     JSTaggedValue,
                                                     JSTaggedValue,
                                                     base::AlignedPointer,
                                                     InterpretedFrameBase> {
public:
    enum class Index : size_t {
        ConstPoolIndex = 0,
        FunctionIndex,
        ThisObjIndex,
        ProFileTypeInfoIndex,
        AccIndex,
        EnvIndex,
        PcIndex,
        BaseIndex,
        NumOfMembers
    };
    static_assert(static_cast<size_t>(Index::NumOfMembers) == NumOfTypes);

    inline JSTaggedType* GetPrevFrameFp() const
    {
        return base.prev;
    }

    static InterpretedFrame* GetFrameFromSp(const JSTaggedType *sp)
    {
        return reinterpret_cast<InterpretedFrame *>(const_cast<JSTaggedType *>(sp)) - 1;
    }

    inline const uint8_t *GetPc() const
    {
        return pc;
    }

    inline JSTaggedValue GetEnv() const
    {
        return env;
    }

    static uint32_t NumOfMembers()
    {
        return sizeof(InterpretedFrame) / JSTaggedValue::TaggedTypeSize();
    }

    static size_t GetPcOffset(bool isArch32)
    {
        return GetOffset<static_cast<size_t>(Index::PcIndex)>(isArch32);
    }

    static size_t GetTypeOffset(bool isArch32 = false)
    {
        return GetOffset<static_cast<size_t>(Index::BaseIndex)>(isArch32) +
            InterpretedFrameBase::GetTypeOffset(isArch32);
    }

    static size_t GetPrevOffset(bool isArch32 = false)
    {
        return GetOffset<static_cast<size_t>(Index::BaseIndex)>(isArch32) +
            InterpretedFrameBase::GetPrevOffset(isArch32);
    }

    static size_t GetFunctionOffset(bool isArch32 = false)
    {
        return GetOffset<static_cast<size_t>(Index::FunctionIndex)>(isArch32);
    }

    void GCIterate(const FrameIterator &it, RootVisitor &visitor) const;

    alignas(EAS) JSTaggedValue constpool {JSTaggedValue::Hole()};
    alignas(EAS) JSTaggedValue function {JSTaggedValue::Hole()};
    alignas(EAS) JSTaggedValue thisObj {JSTaggedValue::Hole()};
    alignas(EAS) JSTaggedValue profileTypeInfo {JSTaggedValue::Hole()};
    alignas(EAS) JSTaggedValue acc {JSTaggedValue::Hole()};
    alignas(EAS) JSTaggedValue env {JSTaggedValue::Hole()};
    alignas(EAS) const uint8_t *pc {nullptr};
    alignas(EAS) InterpretedFrameBase base;
    friend class FrameIterator;
};
STATIC_ASSERT_EQ_ARCH(sizeof(InterpretedFrame), InterpretedFrame::SizeArch32, InterpretedFrame::SizeArch64);

// * InterpretedBuiltinFrame layout description as the following:
//               |--------------------------| ---------------
//               |         . . . . .        |               ^
//               |    InterpretedFrameBase  |               |
//               |         . . . . .        |               |
//               |--------------------------|    InterpretedBuiltinFrame
//               |       bytecode-PC        |               |
//               |--------------------------|               |
//               |       call-target        |               v
//               +--------------------------+ ---------------
//
// NOLINTNEXTLINE(cppcoreguidelines-pro-type-member-init)
struct InterpretedBuiltinFrame : public base::AlignedStruct<JSTaggedValue::TaggedTypeSize(),
                                                            JSTaggedValue,
                                                            base::AlignedPointer,
                                                            InterpretedFrameBase> {
    enum class Index : size_t {
        FunctionIndex = 0,
        PcIndex,
        BaseIndex,
        NumOfMembers
    };
    static_assert(static_cast<size_t>(Index::NumOfMembers) == NumOfTypes);

    inline JSTaggedType* GetPrevFrameFp()
    {
        return base.prev;
    }

    static InterpretedBuiltinFrame* GetFrameFromSp(const JSTaggedType *sp)
    {
        return reinterpret_cast<InterpretedBuiltinFrame *>(const_cast<JSTaggedType *>(sp)) - 1;
    }

    static uint32_t NumOfMembers()
    {
        return sizeof(InterpretedBuiltinFrame) / JSTaggedValue::TaggedTypeSize();
    }

    static size_t GetTypeOffset(bool isArch32 = false)
    {
        return GetOffset<static_cast<size_t>(Index::BaseIndex)>(isArch32) +
            InterpretedFrameBase::GetTypeOffset(isArch32);
    }

    static size_t GetPrevOffset(bool isArch32 = false)
    {
        return GetOffset<static_cast<size_t>(Index::BaseIndex)>(isArch32) +
            InterpretedFrameBase::GetPrevOffset(isArch32);
    }

    static size_t GetFunctionOffset(bool isArch32 = false)
    {
        return GetOffset<static_cast<size_t>(Index::FunctionIndex)>(isArch32);
    }

    void GCIterate(const FrameIterator &it, RootVisitor &visitor) const;

    alignas(EAS) JSTaggedValue function {JSTaggedValue::Hole()};
    alignas(EAS) const uint8_t *pc {nullptr};
    alignas(EAS) InterpretedFrameBase base;
};
STATIC_ASSERT_EQ_ARCH(sizeof(InterpretedBuiltinFrame),
                      InterpretedBuiltinFrame::SizeArch32,
                      InterpretedBuiltinFrame::SizeArch64);

// AsmInterpretedFrame Layout as the following:
//   +----------------------------------+
//   |    argv[n-1]                     |
//   |----------------------------------|
//   |    ......                        |
//   |----------------------------------|
//   |    thisArg [maybe not exist]     |
//   |----------------------------------|
//   |    newTarget [maybe not exist]   |
//   |----------------------------------|
//   |    ......                        |
//   |----------------------------------|
//   |    Vregs [not exist in native]   |
//   +----------------------------------+--------+
//   |        .  .  .   .               |        ^
//   |     InterpretedFrameBase         |        |
//   |        .  .  .   .               |        |
//   |----------------------------------|        |
//   |    pc(bytecode addr)             |        |
//   |----------------------------------|        |
//   |    sp(current stack pointer)     |        |
//   |----------------------------------|   AsmInterpretedFrame
//   |    callSize                      |        |
//   |----------------------------------|        |
//   |    env                           |        |
//   |----------------------------------|        |
//   |    acc                           |        |
//   |----------------------------------|        |
//   |    thisObj                       |        |
//   |----------------------------------|        |
//   |    call-target                   |        v
//   +----------------------------------+--------+
//
// NOLINTNEXTLINE(cppcoreguidelines-pro-type-member-init)
struct AsmInterpretedFrame : public base::AlignedStruct<JSTaggedValue::TaggedTypeSize(),
                                                        JSTaggedValue,
                                                        JSTaggedValue,
                                                        JSTaggedValue,
                                                        JSTaggedValue,
                                                        base::AlignedPointer,
                                                        base::AlignedPointer,
                                                        base::AlignedPointer,
                                                        InterpretedFrameBase> {
    enum class Index : size_t {
        FunctionIndex = 0,
        ThisObjIndex,
        AccIndex,
        EnvIndex,
        CallSizeIndex,
        FpIndex,
        PcIndex,
        BaseIndex,
        NumOfMembers
    };

    static_assert(static_cast<size_t>(Index::NumOfMembers) == NumOfTypes);

    inline JSTaggedType* GetCurrentFramePointer()
    {
        return fp;
    }

    inline JSTaggedType* GetPrevFrameFp()
    {
        return base.prev;
    }

    static AsmInterpretedFrame* GetFrameFromSp(const JSTaggedType *sp)
    {
        return reinterpret_cast<AsmInterpretedFrame *>(const_cast<JSTaggedType *>(sp)) - 1;
    }

    static size_t GetFpOffset(bool isArch32)
    {
        return GetOffset<static_cast<size_t>(Index::FpIndex)>(isArch32);
    }

    static size_t GetCallSizeOffset(bool isArch32)
    {
        return GetOffset<static_cast<size_t>(Index::CallSizeIndex)>(isArch32);
    }

    static size_t GetFunctionOffset(bool isArch32)
    {
        return GetOffset<static_cast<size_t>(Index::FunctionIndex)>(isArch32);
    }

    static size_t GetThisOffset(bool isArch32)
    {
        return GetOffset<static_cast<size_t>(Index::ThisObjIndex)>(isArch32);
    }

    static size_t GetAccOffset(bool isArch32)
    {
        return GetOffset<static_cast<size_t>(Index::AccIndex)>(isArch32);
    }

    static size_t GetEnvOffset(bool isArch32)
    {
        return GetOffset<static_cast<size_t>(Index::EnvIndex)>(isArch32);
    }

    static size_t GetBaseOffset(bool isArch32)
    {
        return GetOffset<static_cast<size_t>(Index::BaseIndex)>(isArch32);
    }

    static size_t GetPcOffset(bool isArch32)
    {
        return GetOffset<static_cast<size_t>(Index::PcIndex)>(isArch32);
    }

    static constexpr size_t GetSize(bool isArch32)
    {
        return isArch32 ? AsmInterpretedFrame::SizeArch32 : AsmInterpretedFrame::SizeArch64;
    }

    static intptr_t GetFpOffsetAsIntptr(bool isArch32)
    {
        return static_cast<intptr_t>(GetFpOffset(isArch32));
    }

    static intptr_t GetFunctionOffsetAsIntptr(bool isArch32)
    {
        return static_cast<intptr_t>(GetFunctionOffset(isArch32));
    }

    static intptr_t GetSizeAsIntptr(bool isArch32)
    {
        return static_cast<intptr_t>(GetSize(isArch32));
    }

    static uint32_t NumOfMembers()
    {
        return sizeof(AsmInterpretedFrame) / JSTaggedValue::TaggedTypeSize();
    }
    void GCIterate(const FrameIterator &it, RootVisitor &visitor, bool isBaselineFrame) const;

    JSTaggedValue GetEnv() const
    {
        return env;
    }

    const uint8_t *GetPc() const
    {
        return pc;
    }

    static size_t GetTypeOffset()
    {
        return MEMBER_OFFSET(AsmInterpretedFrame, base) + MEMBER_OFFSET(InterpretedFrameBase, type);
    }

    static size_t GetPrevOffset()
    {
        return MEMBER_OFFSET(AsmInterpretedFrame, base) + MEMBER_OFFSET(InterpretedFrameBase, prev);
    }

    alignas(EAS) JSTaggedValue function {JSTaggedValue::Hole()};
    alignas(EAS) JSTaggedValue thisObj {JSTaggedValue::Hole()};
    alignas(EAS) JSTaggedValue acc {JSTaggedValue::Hole()};
    alignas(EAS) JSTaggedValue env {JSTaggedValue::Hole()};
    alignas(EAS) uintptr_t callSize {0};
    alignas(EAS) JSTaggedType *fp {nullptr};
    alignas(EAS) const uint8_t *pc {nullptr};
    alignas(EAS) InterpretedFrameBase base;
    // vregs, not exist in native
    // args, may be truncated if not extra
};
STATIC_ASSERT_EQ_ARCH(sizeof(AsmInterpretedFrame), AsmInterpretedFrame::SizeArch32, AsmInterpretedFrame::SizeArch64);

// InterpretedEntryFrame Layout as the following:
//   +----------------------------------+---------------
//   |        .  .  .   .               |              ^
//   |     InterpretedFrameBase         |              |
//   |        .  .  .   .               |    InterpretedEntryFrame
//   |----------------------------------|              |
//   |    pc(bytecode addr)             |              v
//   |----------------------------------|---------------
//
// NOLINTNEXTLINE(cppcoreguidelines-pro-type-member-init)
struct InterpretedEntryFrame : public base::AlignedStruct<JSTaggedValue::TaggedTypeSize(),
                                                          base::AlignedPointer,
                                                          InterpretedFrameBase> {
    enum class Index : size_t {
        PcIndex = 0,
        BaseIndex,
        NumOfMembers
    };
    static_assert(static_cast<size_t>(Index::NumOfMembers) == NumOfTypes);

    inline JSTaggedType* GetPrevFrameFp()
    {
        return base.prev;
    }

    static InterpretedEntryFrame* GetFrameFromSp(const JSTaggedType *sp)
    {
        return reinterpret_cast<InterpretedEntryFrame *>(const_cast<JSTaggedType *>(sp)) - 1;
    }

    static uint32_t NumOfMembers()
    {
        return sizeof(InterpretedEntryFrame) / JSTaggedValue::TaggedTypeSize();
    }

    static size_t GetTypeOffset(bool isArch32 = false)
    {
        return GetOffset<static_cast<size_t>(Index::BaseIndex)>(isArch32) +
            InterpretedFrameBase::GetTypeOffset(isArch32);
    }

    static size_t GetPrevOffset(bool isArch32 = false)
    {
        return GetOffset<static_cast<size_t>(Index::BaseIndex)>(isArch32) +
            InterpretedFrameBase::GetPrevOffset(isArch32);
    }

    void GCIterate(const FrameIterator &it, RootVisitor &visitor) const;
    alignas(EAS) const uint8_t *pc {nullptr};
    alignas(EAS) InterpretedFrameBase base;
};
STATIC_ASSERT_EQ_ARCH(sizeof(InterpretedEntryFrame),
                      InterpretedEntryFrame::SizeArch32,
                      InterpretedEntryFrame::SizeArch64);


// AsmInterpretedEntryFrame Layout as the following:
//   +----------------------------------+---------------
//   |        .  .  .   .               |              ^
//   |     InterpretedFrameBase         |              |
//   |        .  .  .   .               |    AsmInterpretedEntryFrame
//   |----------------------------------|              |
//   |    pc(bytecode addr)             |              v
//   |----------------------------------|---------------
//
// NOLINTNEXTLINE(cppcoreguidelines-pro-type-member-init)
struct AsmInterpretedEntryFrame : public base::AlignedStruct<JSTaggedValue::TaggedTypeSize(),
                                                             base::AlignedPointer,
                                                             InterpretedFrameBase> {
    enum class Index : size_t {
        PcIndex = 0,
        BaseIndex,
        NumOfMembers
    };
    static_assert(static_cast<size_t>(Index::NumOfMembers) == NumOfTypes);

    inline JSTaggedType* GetPrevFrameFp()
    {
        return base.prev;
    }

    static size_t GetBaseOffset(bool isArch32)
    {
        return GetOffset<static_cast<size_t>(Index::BaseIndex)>(isArch32);
    }

    static AsmInterpretedEntryFrame* GetFrameFromSp(const JSTaggedType *sp)
    {
        return reinterpret_cast<AsmInterpretedEntryFrame *>(const_cast<JSTaggedType *>(sp)) - 1;
    }

    static size_t GetTypeOffset(bool isArch32 = false)
    {
        return GetOffset<static_cast<size_t>(Index::BaseIndex)>(isArch32) +
            InterpretedFrameBase::GetTypeOffset(isArch32);
    }

    static size_t GetPrevOffset(bool isArch32 = false)
    {
        return GetOffset<static_cast<size_t>(Index::BaseIndex)>(isArch32) +
            InterpretedFrameBase::GetPrevOffset(isArch32);
    }

    alignas(EAS) const uint8_t *pc {nullptr};
    alignas(EAS) InterpretedFrameBase base;
};

// AsmInterpretedBridgeFrame Layout as the following:
//   +----------------------------------+---------------
//   |      ret-address                 |              ^
//   |----------------------------------|              |
//   |        .  .  .   .               |     AsmInterpretedBridgeFrame
//   |     AsmInterpretedEntryFrame     |              |
//   |        .  .  .   .               |              v
//   |----------------------------------|---------------
//
// NOLINTNEXTLINE(cppcoreguidelines-pro-type-member-init)
struct AsmInterpretedBridgeFrame : public base::AlignedStruct<base::AlignedPointer::Size(),
                                                              AsmInterpretedEntryFrame,
                                                              base::AlignedPointer> {
    enum class Index : size_t {
        EntryIndex = 0,
        ReturnAddrIndex,
        NumOfMembers
    };
    static_assert(static_cast<size_t>(Index::NumOfMembers) == NumOfTypes);

    static AsmInterpretedBridgeFrame* GetFrameFromSp(const JSTaggedType *sp)
    {
        return reinterpret_cast<AsmInterpretedBridgeFrame *>(reinterpret_cast<uintptr_t>(sp) -
            MEMBER_OFFSET(AsmInterpretedBridgeFrame, returnAddr));
    }
    uintptr_t GetCallSiteSp() const
    {
        return ToUintPtr(this) + sizeof(AsmInterpretedBridgeFrame);
    }
    inline JSTaggedType* GetPrevFrameFp()
    {
        return entry.base.prev;
    }

    static size_t GetReturnAddrOffset(bool isArch32)
    {
        return GetOffset<static_cast<size_t>(Index::ReturnAddrIndex)>(isArch32);
    }

    static constexpr size_t GetSize(bool isArch32)
    {
        return isArch32 ? AsmInterpretedBridgeFrame::SizeArch32 : AsmInterpretedBridgeFrame::SizeArch64;
    }

    static size_t GetTypeOffset(bool isArch32 = false)
    {
        return GetOffset<static_cast<size_t>(Index::EntryIndex)>(isArch32) +
            AsmInterpretedEntryFrame::GetBaseOffset(isArch32) +
            InterpretedFrameBase::GetTypeOffset(isArch32);
    }
    static size_t GetPrevOffset(bool isArch32 = false)
    {
        return GetOffset<static_cast<size_t>(Index::EntryIndex)>(isArch32) +
            AsmInterpretedEntryFrame::GetBaseOffset(isArch32) +
            InterpretedFrameBase::GetPrevOffset(isArch32);
    }

    uintptr_t GetReturnAddr() const
    {
        return returnAddr;
    }

    AsmInterpretedEntryFrame entry;
    alignas(EAS) uintptr_t returnAddr {0};
};

// * Optimized-leaved-frame layout as the following:
//               +--------------------------+
//               |       argv[N-1]          |
//               |--------------------------|
//               |       . . . . .          |
//               |--------------------------|
//               |       argv[0]            |
//               +--------------------------+-------------
//               |       argc               |            ^
//               |--------------------------|            |
//               |       RuntimeId          |            |
//  callerSP --> |--------------------------|   OptimizedLeaveFrame
//               |       ret-addr           |            |
//               |--------------------------|            |
//               |       prevFp             |            |
//        fp --> |--------------------------|            |
//               |       frameType          |            v
//  calleeSP --> +--------------------------+-------------
//
// NOLINTNEXTLINE(cppcoreguidelines-pro-type-member-init)
struct OptimizedLeaveFrame : public base::AlignedStruct<JSTaggedValue::TaggedTypeSize(),
                                                        base::AlignedPointer,
                                                        base::AlignedPointer,
                                                        base::AlignedPointer,
                                                        base::AlignedUint64,
                                                        base::AlignedUint64> {
    // argv[0]...argv[argc-1] dynamic according to agc
    static OptimizedLeaveFrame* GetFrameFromSp(const JSTaggedType *sp)
    {
        return reinterpret_cast<OptimizedLeaveFrame *>(reinterpret_cast<uintptr_t>(sp) -
            MEMBER_OFFSET(OptimizedLeaveFrame, callsiteFp));
    }

    uintptr_t GetCallSiteSp() const
    {
        return ToUintPtr(this) + MEMBER_OFFSET(OptimizedLeaveFrame, argRuntimeId);
    }

    inline JSTaggedType* GetPrevFrameFp() const
    {
        return reinterpret_cast<JSTaggedType*>(callsiteFp);
    }

    uintptr_t GetReturnAddr() const
    {
        return returnAddr;
    }

    static size_t GetTypeOffset()
    {
        return MEMBER_OFFSET(OptimizedLeaveFrame, type);
    }

    static size_t GetPrevOffset()
    {
        return MEMBER_OFFSET(OptimizedLeaveFrame, callsiteFp);
    }

    static size_t GetReturnAddrOffset()
    {
        return MEMBER_OFFSET(OptimizedLeaveFrame, returnAddr);
    }

    void GCIterate(const FrameIterator &it, RootVisitor &visitor) const;

    alignas(EAS) FrameType type {0};
    alignas(EAS) uintptr_t callsiteFp {0}; // thread sp set here
    alignas(EAS) uintptr_t returnAddr {0};
    alignas(EAS) uint64_t argRuntimeId {0};
    alignas(EAS) uint64_t argc {0};
};

// * Optimized-leaved-frame-with-argv layout as the following:
//               +--------------------------+
//               |       argv[]             |
//               +--------------------------+-------------
//               |       argc               |            ^
//               |--------------------------|            |
//               |       RuntimeId          |   OptimizedWithArgvLeaveFrame
//  callerSP --> |--------------------------|            |
//               |       returnAddr         |            |
//               |--------------------------|            |
//               |       callsiteFp         |            |
//        fp --> |--------------------------|            |
//               |       frameType          |            v
//  calleeSP --> +--------------------------+-------------

struct OptimizedWithArgvLeaveFrame : public base::AlignedStruct<JSTaggedValue::TaggedTypeSize(),
                                                                base::AlignedPointer,
                                                                base::AlignedPointer,
                                                                base::AlignedPointer,
                                                                base::AlignedUint64,
                                                                base::AlignedUint64> {
    static OptimizedWithArgvLeaveFrame* GetFrameFromSp(const JSTaggedType *sp)
    {
        return reinterpret_cast<OptimizedWithArgvLeaveFrame *>(reinterpret_cast<uintptr_t>(sp) -
            MEMBER_OFFSET(OptimizedWithArgvLeaveFrame, callsiteFp));
    }

    uintptr_t GetCallSiteSp() const
    {
        return ToUintPtr(this) + MEMBER_OFFSET(OptimizedWithArgvLeaveFrame, argRuntimeId);
    }

    inline JSTaggedType* GetPrevFrameFp()
    {
        return reinterpret_cast<JSTaggedType*>(callsiteFp);
    }

    uintptr_t GetReturnAddr() const
    {
        return returnAddr;
    }

    static size_t GetTypeOffset()
    {
        return MEMBER_OFFSET(OptimizedWithArgvLeaveFrame, type);
    }

    static size_t GetPrevOffset()
    {
        return MEMBER_OFFSET(OptimizedWithArgvLeaveFrame, callsiteFp);
    }

    static size_t GetReturnAddrOffset()
    {
        return MEMBER_OFFSET(OptimizedWithArgvLeaveFrame, returnAddr);
    }

    void GCIterate(const FrameIterator &it, RootVisitor &visitor) const;

    alignas(EAS) FrameType type {0};
    alignas(EAS) uintptr_t callsiteFp {0}; // thread sp set here
    alignas(EAS) uintptr_t returnAddr {0};
    alignas(EAS) uint64_t argRuntimeId {0};
    alignas(EAS) uint64_t argc {0};
};

// * Optimized-builtin-leave-frame layout as the following:
//               +--------------------------+
//               |       argv[N-1]          |
//               |--------------------------|
//               |       . . . . .          |
//               |--------------------------|
//               |       argv[0]            |
//               +--------------------------+-------------
//               |       argc               |            ^
//               |--------------------------|            |
//               |       thread             |            |
//  callerSP --> +--------------------------+            |
//               |       ret-addr           |            |
//               |--------------------------|   OptimizedBuiltinLeaveFrame
//               |       prevFp             |            |
//        fp --> |--------------------------|            |
//               |       frameType          |            |
//               |--------------------------|            |
//               |       align byte         |            v
//  calleeSP --> +--------------------------+-------------
//
// NOLINTNEXTLINE(cppcoreguidelines-pro-type-member-init)
struct OptimizedBuiltinLeaveFrame : public base::AlignedStruct<JSTaggedValue::TaggedTypeSize(),
                                                               base::AlignedPointer,
                                                               base::AlignedPointer,
                                                               base::AlignedPointer,
                                                               JSTaggedValue,
                                                               base::AlignedUint64> {
public:
    static OptimizedBuiltinLeaveFrame* GetFrameFromSp(const JSTaggedType *sp)
    {
        return reinterpret_cast<OptimizedBuiltinLeaveFrame *>(reinterpret_cast<uintptr_t>(sp) -
            MEMBER_OFFSET(OptimizedBuiltinLeaveFrame, callsiteFp));
    }

    uintptr_t GetCallSiteSp() const
    {
        return ToUintPtr(this) + MEMBER_OFFSET(OptimizedBuiltinLeaveFrame, thread);
    }

    inline JSTaggedType* GetPrevFrameFp() const
    {
        return reinterpret_cast<JSTaggedType*>(callsiteFp);
    }

    uintptr_t GetReturnAddr() const
    {
        return returnAddr;
    }

    void GCIterate(const FrameIterator &it, RootVisitor &visitor) const;

    static size_t GetTypeOffset()
    {
        return MEMBER_OFFSET(OptimizedBuiltinLeaveFrame, type);
    }

    static size_t GetPrevOffset()
    {
        return MEMBER_OFFSET(OptimizedBuiltinLeaveFrame, callsiteFp);
    }

    static size_t GetReturnAddrOffset()
    {
        return MEMBER_OFFSET(OptimizedBuiltinLeaveFrame, returnAddr);
    }

    static size_t GetFunctionOffset()
    {
        return MEMBER_OFFSET(OptimizedBuiltinLeaveFrame, argc) + 1;
    }

    const JSTaggedType* GetArgv() const
    {
        return reinterpret_cast<const JSTaggedType *>(&argc + 1);
    }

    FrameType GetType() const
    {
        return type;
    }

private:
    alignas(EAS) FrameType type {0};
    alignas(EAS) uintptr_t callsiteFp {0}; // thread sp set here
    alignas(EAS) uintptr_t returnAddr {0};
    alignas(EAS) JSTaggedValue thread {JSTaggedValue::Hole()};
    alignas(EAS) uint64_t argc {0};
    // argv[0]...argv[argc-1] dynamic according to agc
};

// * BuiltinFrame layout as the following:
//               +--------------------------+
//               |     argV[N - 1]          |
//               |--------------------------|
//               |       . . . .            |
//               |--------------------------+
//               |     argV[2]=this         |
//               +--------------------------+
//               |     argV[1]=new-target   |
//               +--------------------------+
//               |     argV[0]=call-target  |
//               +--------------------------+ ---------
//               |       argc               |         ^
//               |--------------------------|         |
//               |       thread             |         |
//  callerSP --> |--------------------------|         |
//               |       returnAddr         |     BuiltinFrame
//               |--------------------------|         |
//               |       callsiteFp         |         |
//        fp --> |--------------------------|         |
//               |       frameType          |         v
//  calleeSP --> +--------------------------+ ---------
//
// NOLINTNEXTLINE(cppcoreguidelines-pro-type-member-init)
struct BuiltinFrame : public base::AlignedStruct<base::AlignedPointer::Size(),
                                                 base::AlignedSize,
                                                 base::AlignedPointer,
                                                 base::AlignedPointer,
                                                 base::AlignedPointer,
                                                 base::AlignedUint32,
                                                 base::AlignedPointer> {
    enum class Index : size_t {
        TypeIndex = 0,
        PrevFpIndex,
        ReturnAddrIndex,
        ThreadIndex,
        NumArgsIndex,
        StackArgsIndex,
        NumOfMembers
    };
    static_assert(static_cast<size_t>(Index::NumOfMembers) == NumOfTypes);

    static BuiltinFrame* GetFrameFromSp(const JSTaggedType *sp)
    {
        return reinterpret_cast<BuiltinFrame *>(reinterpret_cast<uintptr_t>(sp) -
            MEMBER_OFFSET(BuiltinFrame, prevFp));
    }

    inline JSTaggedType* GetPrevFrameFp()
    {
        return prevFp;
    }

    uintptr_t GetCallSiteSp() const
    {
        return ToUintPtr(this) + MEMBER_OFFSET(BuiltinFrame, thread);
    }

    static size_t GetPreFpOffset(bool isArch32)
    {
        return GetOffset<static_cast<size_t>(Index::PrevFpIndex)>(isArch32);
    }

    static size_t GetNumArgsToFpDelta(bool isArch32)
    {
        auto offset = GetOffset<static_cast<size_t>(Index::NumArgsIndex)>(isArch32);
        return offset - GetPreFpOffset(isArch32);
    }

    static size_t GetStackArgsToFpDelta(bool isArch32)
    {
        auto offset = GetOffset<static_cast<size_t>(Index::StackArgsIndex)>(isArch32);
        return offset - GetPreFpOffset(isArch32);
    }

    uintptr_t GetStackArgsAddress()
    {
        return reinterpret_cast<uintptr_t>(&stackArgs);
    }

    JSTaggedValue GetFunction()
    {
        auto functionAddress = reinterpret_cast<JSTaggedType *>(GetStackArgsAddress());
        return JSTaggedValue(*functionAddress);
    }

    uint32_t GetNumArgs()
    {
        return numArgs;
    }

    uintptr_t GetReturnAddr() const
    {
        return returnAddr;
    }

    static size_t GetStackArgsOffset(bool isArch32 = false)
    {
        return GetOffset<static_cast<size_t>(Index::StackArgsIndex)>(isArch32);
    }

    static size_t GetTypeOffset(bool isArch32 = false)
    {
        return GetOffset<static_cast<size_t>(Index::TypeIndex)>(isArch32);
    }

    static size_t GetPrevOffset(bool isArch32 = false)
    {
        return GetOffset<static_cast<size_t>(Index::PrevFpIndex)>(isArch32);
    }

    static size_t GetReturnAddrOffset(bool isArch32 = false)
    {
        return GetOffset<static_cast<size_t>(Index::ReturnAddrIndex)>(isArch32);
    }

    void GCIterate(const FrameIterator &it, RootVisitor &visitor) const;

    alignas(EAS) FrameType type {0};
    alignas(EAS) JSTaggedType *prevFp {nullptr};
    alignas(EAS) uintptr_t returnAddr {0};
    alignas(EAS) uintptr_t thread {0};
    alignas(EAS) uint32_t numArgs {0};
    alignas(EAS) uintptr_t stackArgs {0};
};

// * BuiltinWithArgvFrame layout as the following:
//               +--------------------------+ ---------
//               |       . . . . .          |         ^
//  callerSP --> |--------------------------|         |
//               |       returnAddr         |         |
//               |--------------------------|         |
//               |       callsiteFp         |   BuiltinWithArgvFrame
//        fp --> |--------------------------|         |
//               |       frameType          |         |
//               +--------------------------+         |
//               |        argc              |         v
//               +--------------------------+ ---------
//               |        argV[0]           |
//               +--------------------------+
//               |        argV[1]           |
//               +--------------------------+
//               |        . . . .           |
//  calleeSP --> +--------------------------+
//
// NOLINTNEXTLINE(cppcoreguidelines-pro-type-member-init)
struct BuiltinWithArgvFrame : public base::AlignedStruct<base::AlignedPointer::Size(),
                                                         base::AlignedSize,
                                                         base::AlignedPointer,
                                                         base::AlignedPointer> {
    enum class Index : int {
        StackArgsTopIndex = -1,
        NumArgsIndex = -1,
        TypeIndex = 0,
        PrevFpIndex,
        ReturnAddrIndex,
        NumOfMembers
    };
    static_assert(static_cast<size_t>(Index::NumOfMembers) == NumOfTypes);

    static BuiltinWithArgvFrame* GetFrameFromSp(const JSTaggedType *sp)
    {
        return reinterpret_cast<BuiltinWithArgvFrame *>(reinterpret_cast<uintptr_t>(sp) -
            MEMBER_OFFSET(BuiltinFrame, prevFp));
    }

    inline JSTaggedType* GetPrevFrameFp()
    {
        return prevFp;
    }

    uintptr_t GetCallSiteSp() const
    {
        return ToUintPtr(this) + sizeof(BuiltinWithArgvFrame);
    }

    uintptr_t GetStackArgsAddress()
    {
        auto topAddress = ToUintPtr(this) +
            (static_cast<int>(Index::StackArgsTopIndex) * sizeof(uintptr_t));
        auto numberArgs = GetNumArgs() + NUM_MANDATORY_JSFUNC_ARGS;
        return topAddress - static_cast<uint32_t>(numberArgs) * sizeof(uintptr_t);
    }

    JSTaggedValue GetFunction()
    {
        auto functionAddress = reinterpret_cast<JSTaggedType *>(GetStackArgsAddress());
        return JSTaggedValue(*functionAddress);
    }

    int32_t GetNumArgs()
    {
        auto argcAddress = reinterpret_cast<int32_t *>(
            ToUintPtr(this) + (static_cast<int>(Index::NumArgsIndex) * sizeof(uintptr_t)));
        return *argcAddress;
    }

    uintptr_t GetReturnAddr() const
    {
        return returnAddr;
    }

    static size_t GetTypeOffset(bool isArch32 = false)
    {
        return GetOffset<static_cast<size_t>(Index::TypeIndex)>(isArch32);
    }

    static size_t GetPrevOffset(bool isArch32 = false)
    {
        return GetOffset<static_cast<size_t>(Index::PrevFpIndex)>(isArch32);
    }

    static size_t GetReturnAddrOffset(bool isArch32 = false)
    {
        return GetOffset<static_cast<size_t>(Index::ReturnAddrIndex)>(isArch32);
    }

    void GCIterate(const FrameIterator &it, RootVisitor &visitor) const;
    // argv(... this, new.target, function)
    // numargs
    alignas(EAS) FrameType type {0};
    alignas(EAS) JSTaggedType *prevFp {nullptr};
    alignas(EAS) uintptr_t returnAddr {0};
};

// * FASTJITFunctionFrame layout description as the following:
//               +--------------------------+
//               |        arg[N-1]          |
//               +--------------------------+
//               |       ...                |
//               +--------------------------+
//               |       arg[1]             |
//               +--------------------------+
//               |       arg[0]             |
//               +--------------------------+
//               |       this               |
//               +--------------------------+
//               |       new-target         |
//               +--------------------------+
//               |       call-target        |
//               |--------------------------|
//               |       argc               |
// callerSp ---> |--------------------------| ---------------
//               |       returnAddr         |               ^
//               |--------------------------|               |
//               |       callsiteFp         |               |
//               |--------------------------|               |
//               |       frameType          |    FASTJITFunctionFrame
//               |--------------------------|               |
//               |       call-target        |               |
//               |--------------------------|               |
//               |       pc(bytecode pc)    |               v
// calleeSP ---> +--------------------------+ ---------------
//
// NOLINTNEXTLINE(cppcoreguidelines-pro-type-member-init)
struct FASTJITFunctionFrame : public base::AlignedStruct<JSTaggedValue::TaggedTypeSize(),
                                                             JSTaggedValue,
                                                             JSTaggedValue,
                                                             base::AlignedPointer,
                                                             base::AlignedPointer,
                                                             base::AlignedPointer> {
public:
    using ConstInfo = kungfu::LLVMStackMapType::ConstInfo;
    enum class Index : size_t {
        PcIndex = 0,
        JSFuncIndex,
        TypeIndex,
        PrevFpIndex,
        ReturnAddrIndex,
        NumOfMembers
    };
    static_assert(static_cast<size_t>(Index::NumOfMembers) == NumOfTypes);

    static constexpr size_t GetFunctionDeltaReturnAddr()
    {
        return static_cast<size_t>(Index::ReturnAddrIndex) - static_cast<size_t>(Index::JSFuncIndex);
    }

    inline JSTaggedType* GetPrevFrameFp()
    {
        return prevFp;
    }

    JSTaggedType* GetArgv(uintptr_t *preFrameSp) const
    {
        const size_t offset = 2;    // 2: skip argc and argv.
        return reinterpret_cast<JSTaggedType *>(preFrameSp + offset * sizeof(uint64_t) / sizeof(uintptr_t));
    }

    size_t GetArgc(uintptr_t *preFrameSp) const
    {
        return *preFrameSp;
    }

    JSTaggedType* GetArgv(const FrameIterator &it) const;

    uintptr_t GetReturnAddr() const
    {
        return returnAddr;
    }

    void GCIterate(const FrameIterator &it, RootVisitor &visitor, FrameType frameType) const;
    void CollectPcOffsetInfo(const FrameIterator &it, ConstInfo &info) const;

    inline JSTaggedValue GetFunction() const
    {
        return jsFunc;
    }

    static uintptr_t ComputeArgsConfigFrameSp(JSTaggedType *fp)
    {
        const size_t offset = 2;  // 2: skip prevFp and return address.
        return reinterpret_cast<uintptr_t>(fp) + offset * sizeof(uintptr_t);
    }

    static size_t GetTypeOffset(bool isArch32 = false)
    {
        return GetOffset<static_cast<size_t>(Index::TypeIndex)>(isArch32);
    }

    static size_t GetPcOffset(bool isArch32 = false)
    {
        return GetOffset<static_cast<size_t>(Index::PcIndex)>(isArch32);
    }

    static size_t GetPrevOffset(bool isArch32 = false)
    {
        return GetOffset<static_cast<size_t>(Index::PrevFpIndex)>(isArch32);
    }

    static size_t GetFunctionOffset(bool isArch32 = false)
    {
        return GetOffset<static_cast<size_t>(Index::JSFuncIndex)>(isArch32);
    }

    static size_t ComputeReservedJSFuncOffset(size_t slotSize)
    {
        size_t slotOffset = static_cast<size_t>(Index::PrevFpIndex) - static_cast<size_t>(Index::JSFuncIndex);
        return slotSize * slotOffset;
    }

    static size_t ComputeReservedPcOffset(size_t slotSize)
    {
        size_t slotOffset = static_cast<size_t>(Index::PrevFpIndex) - static_cast<size_t>(Index::PcIndex);
        return slotSize * slotOffset;
    }

    static int GetFrmaeTypeToFpDelta()
    {
        return -(int)sizeof(uintptr_t);
    }

    static int GetFunctionToFpDelta()
    {
        int slotOffset = static_cast<int>(Index::JSFuncIndex) - static_cast<int>(Index::TypeIndex);
        return slotOffset * static_cast<int>(JSTaggedValue::TaggedTypeSize()) + GetFrmaeTypeToFpDelta();
    }

    FrameType GetType() const
    {
        return type;
    }

    inline const uint8_t *GetPc() const
    {
        return pc;
    }

    friend class FrameIterator;
    friend class FrameHandler;
    void GetDeoptBundleInfo(const FrameIterator &it, std::vector<kungfu::ARKDeopt>& deopts) const;
    void GetFuncCalleeRegAndOffset(
        const FrameIterator &it, kungfu::CalleeRegAndOffsetVec &ret) const;
    uintptr_t* ComputePrevFrameSp(const FrameIterator &it) const;

private:
    static FASTJITFunctionFrame* GetFrameFromSp(const JSTaggedType *sp)
    {
        return reinterpret_cast<FASTJITFunctionFrame *>(reinterpret_cast<uintptr_t>(sp) -
            MEMBER_OFFSET(FASTJITFunctionFrame, prevFp));
    }

    static uintptr_t GetFuncAddrFromSp(const JSTaggedType *sp)
    {
        return reinterpret_cast<uintptr_t>(sp) - MEMBER_OFFSET(FASTJITFunctionFrame, type);
    }

    // dynamic callee saveregisters for x86-64
    alignas(EAS) const uint8_t *pc {nullptr};
    alignas(EAS) JSTaggedValue jsFunc {JSTaggedValue::Undefined()};
    alignas(EAS) FrameType type {0};
    alignas(EAS) JSTaggedType *prevFp {nullptr};
    alignas(EAS) uintptr_t returnAddr {0};
    // dynamic callee saveregisters for arm64
};

enum class GCVisitedFlag : uint8_t {
    VISITED = 0,
    IGNORED,
    HYBRID_STACK,
    DEOPT,
};

class FrameIterator {
public:
    using ConstInfo = kungfu::LLVMStackMapType::ConstInfo;
    using CallSiteInfo = std::tuple<uint64_t, uint8_t *, int, kungfu::CalleeRegAndOffsetVec>;
    explicit FrameIterator(JSTaggedType *sp, const JSThread *thread = nullptr);
    FrameType GetFrameType() const
    {
        ASSERT(current_ != nullptr);
        FrameType *typeAddr = reinterpret_cast<FrameType *>(
            reinterpret_cast<uintptr_t>(current_) - sizeof(FrameType));
        return *typeAddr;
    }

    template<class T>
    T* GetFrame()
    {
        return T::GetFrameFromSp(current_);
    }

    template<class T>
    const T* GetFrame() const
    {
        return T::GetFrameFromSp(current_);
    }

    bool Done() const
    {
        return current_ == nullptr;
    }
    JSTaggedType *GetSp() const
    {
        return current_;
    }
    JSTaggedType *GetSp()
    {
        return current_;
    }
    void GetCalleeRegAndOffsetVec(kungfu::CalleeRegAndOffsetVec &ret) const
    {
        ret = calleeRegInfo_;
    }
    int ComputeDelta(const Method *method = nullptr) const;
    template <GCVisitedFlag GCVisit = GCVisitedFlag::IGNORED>
    void Advance();
    std::map<uint32_t, uint32_t> GetInlinedMethodInfo();
    uint32_t GetBytecodeOffset() const;
    uintptr_t GetPrevFrameCallSiteSp() const;
    uintptr_t GetPrevFrame() const;
    uintptr_t GetCallSiteSp() const
    {
        return optimizedCallSiteSp_;
    }
    uintptr_t GetOptimizedReturnAddr() const
    {
        return optimizedReturnAddr_;
    }
    const JSThread *GetThread() const
    {
        return thread_;
    }
    bool IteratorStackMap(RootVisitor &visitor) const;
    void CollectPcOffsetInfo(ConstInfo &info) const;
    void CollectMethodOffsetInfo(std::map<uint32_t, uint32_t> &info) const;
    void CollectArkDeopt(std::vector<kungfu::ARKDeopt>& deopts) const;
    std::pair<CallSiteInfo, bool> CalCallSiteInfo(uintptr_t retAddr, bool isDeopt) const;
    CallSiteInfo TryCalCallSiteInfoFromMachineCode(uintptr_t retAddr) const;

    Method *CheckAndGetMethod() const;
    JSTaggedValue GetFunction() const;

    bool IsLeaveFrame() const
    {
        FrameType type = GetFrameType();
        return (type == FrameType::LEAVE_FRAME) || (type == FrameType::LEAVE_FRAME_WITH_ARGV);
    }

    bool IsOptimizedFrame() const
    {
        FrameType type = GetFrameType();
        return (type == FrameType::OPTIMIZED_FRAME);
    }

    bool IsInterpretedFrame(FrameType type) const
    {
        return (type >= FrameType::INTERPRETER_FIRST) && (type <= FrameType::INTERPRETER_LAST);
    }

    bool IsJSFrame() const
    {
        FrameType type = GetFrameType();
        return IsInterpretedFrame(type) || IsOptimizedJSFunctionFrame(type) || IsFastJitFunctionFrame(type);
    }

    bool IsOptimizedJSFunctionFrame(FrameType type) const
    {
        return type == FrameType::OPTIMIZED_JS_FUNCTION_FRAME ||
            type == FrameType::OPTIMIZED_JS_FAST_CALL_FUNCTION_FRAME;
    }

    bool IsOptimizedJSFunctionFrame() const
    {
        FrameType type = GetFrameType();
        return IsOptimizedJSFunctionFrame(type);
    }

    bool IsFastJitFunctionFrame(FrameType type) const
    {
        return type == FrameType::FASTJIT_FUNCTION_FRAME ||
            type == FrameType::FASTJIT_FAST_CALL_FUNCTION_FRAME;
    }

    bool IsFastJitFunctionFrame() const
    {
        FrameType type = GetFrameType();
        return IsFastJitFunctionFrame(type);
    }

    bool IsAotOrJitFunctionFrame() const
    {
        return IsOptimizedJSFunctionFrame() || IsFastJitFunctionFrame();
    }

    JSTaggedType *GetMachineCodeSlot() const
    {
        return const_cast<JSTaggedType*>(&machineCode_);
    }

private:
    uint32_t GetBaselineBytecodeOffset() const;

    JSTaggedType *current_ {nullptr};
    const JSThread *thread_ {nullptr};
    const kungfu::ArkStackMapParser *arkStackMapParser_ {nullptr};
    uintptr_t optimizedCallSiteSp_ {0};
    uintptr_t optimizedReturnAddr_ {0};
    uint8_t *stackMapAddr_ {nullptr};
    kungfu::CalleeRegAndOffsetVec calleeRegInfo_;

    // in jit, delta on method is not set, get it from iterator
    bool isJITFrame_ {false};
    int fpDeltaPrevFrameSp_ {0};

    // cache current machine code, it's nonmovable
    JSTaggedType machineCode_ {JSTaggedValue::VALUE_UNDEFINED};
    uintptr_t baselineNativePc_ {0}; // For baselineJit upFrame
};
}  // namespace panda::ecmascript
#endif // ECMASCRIPT_FRAMES_H
