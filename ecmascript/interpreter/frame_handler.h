/*
 * Copyright (c) 2021-2024 Huawei Device Co., Ltd.
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

#ifndef ECMASCRIPT_INTERPRETER_FRAME_HANDLER_H
#define ECMASCRIPT_INTERPRETER_FRAME_HANDLER_H

#include "ecmascript/frames.h"
#include "ecmascript/js_tagged_value.h"
#include "ecmascript/js_thread.h"
#include "ecmascript/mem/heap.h"
#include "ecmascript/mem/visitor.h"
#include "ecmascript/method.h"

namespace panda {
namespace ecmascript {
class JSThread;
class ConstantPool;
namespace kungfu {
    class ArkStackMapParser;
};

class FrameHandler {
public:
    explicit FrameHandler(const JSThread *thread);

    ~FrameHandler() = default;

    DEFAULT_COPY_SEMANTIC(FrameHandler);
    DEFAULT_MOVE_SEMANTIC(FrameHandler);

    bool HasFrame() const
    {
        return sp_ != nullptr;
    }

    inline static FrameType GetFrameType(const JSTaggedType *sp)
    {
        ASSERT(sp != nullptr);
        FrameType *typeAddr = reinterpret_cast<FrameType *>(reinterpret_cast<uintptr_t>(sp) - sizeof(FrameType));
        return *typeAddr;
    }

    inline static bool IsEntryFrame(const uint8_t *pc)
    {
        return pc == nullptr;
    }

    bool IsEntryFrame() const
    {
        ASSERT(HasFrame());
        // The structure of InterpretedFrame, AsmInterpretedFrame, InterpretedEntryFrame is the same, order is pc, base.
        FrameType type = GetFrameType();
        if (type == FrameType::BUILTIN_ENTRY_FRAME ||
            type == FrameType::INTERPRETER_ENTRY_FRAME ||
            type == FrameType::ASM_INTERPRETER_ENTRY_FRAME ||
            type == FrameType::OPTIMIZED_ENTRY_FRAME) {
                return true;
        } else if (type == FrameType::ASM_INTERPRETER_BRIDGE_FRAME ||
                   type == FrameType::INTERPRETER_FRAME) {
            FrameIterator it(sp_);
            InterpretedFrame *state = it.GetFrame<InterpretedFrame>();
            return state->GetPc() == nullptr;
        }
        return false;
    }

    bool IsInterpretedFrame() const
    {
        FrameType type = GetFrameType();
        return IsInterpretedFrame(type);
    }

    bool IsInterpretedFrame(FrameType type) const
    {
        return (type >= FrameType::INTERPRETER_FIRST) && (type <= FrameType::INTERPRETER_LAST);
    }

    bool IsJSFrame() const
    {
        FrameType type = GetFrameType();
        return IsJSFrame(type);
    }

    bool IsOptimizedJSFunctionFrame() const
    {
        FrameType type = GetFrameType();
        return type == FrameType::OPTIMIZED_JS_FUNCTION_FRAME ||
            type == FrameType::OPTIMIZED_JS_FAST_CALL_FUNCTION_FRAME;
    }

    bool IsJSFrame(FrameType type) const
    {
        return IsInterpretedFrame(type) || IsOptimizedJSFunctionFrame(type) || IsFastJitFunctionFrame(type);
    }

    bool IsOptimizedJSFunctionFrame(FrameType type) const
    {
        return type == FrameType::OPTIMIZED_JS_FUNCTION_FRAME ||
            type == FrameType::OPTIMIZED_JS_FAST_CALL_FUNCTION_FRAME;
    }

    bool IsFastJitFunctionFrame(FrameType type) const
    {
        return type == FrameType::FASTJIT_FUNCTION_FRAME ||
            type == FrameType::FASTJIT_FAST_CALL_FUNCTION_FRAME;
    }

    bool IsAsmInterpretedFrame() const
    {
        FrameIterator it(sp_, thread_);
        FrameType type = it.GetFrameType();
        return (type == FrameType::ASM_INTERPRETER_FRAME) ||
            (type == FrameType::INTERPRETER_CONSTRUCTOR_FRAME);
    }

    bool IsAsmInterpretedFrame(FrameType type) const
    {
        return (type == FrameType::ASM_INTERPRETER_FRAME) ||
            (type == FrameType::INTERPRETER_CONSTRUCTOR_FRAME);
    }
    bool IsBuiltinFrame() const
    {
        FrameType type = GetFrameType();
        return (type >= FrameType::BUILTIN_FIRST) && (type <= FrameType::BUILTIN_LAST);
    }
    bool IsBuiltinEntryFrame() const
    {
        return (GetFrameType() == FrameType::BUILTIN_ENTRY_FRAME);
    }

    bool IsInterpretedEntryFrame() const
    {
        if (thread_->IsAsmInterpreter()) {
            FrameType type = GetFrameType();
            return (type == FrameType::ASM_INTERPRETER_ENTRY_FRAME || type == FrameType::ASM_INTERPRETER_BRIDGE_FRAME);
        }
        return (GetFrameType() == FrameType::INTERPRETER_ENTRY_FRAME);
    }

    bool IsInterpretedEntryFrame(FrameType type) const
    {
        if (thread_->IsAsmInterpreter()) {
            return (type == FrameType::ASM_INTERPRETER_ENTRY_FRAME || type == FrameType::ASM_INTERPRETER_BRIDGE_FRAME);
        }
        return (type == FrameType::INTERPRETER_ENTRY_FRAME);
    }

    bool IsAsmInterpretedEntryFrame() const
    {
        FrameType type = GetFrameType();
        return IsAsmInterpretedEntryFrame(type);
    }

    bool IsAsmInterpretedEntryFrame(FrameType type) const
    {
        return (type == FrameType::ASM_INTERPRETER_ENTRY_FRAME || type == FrameType::ASM_INTERPRETER_BRIDGE_FRAME);
    }

    bool IsCInterpretedEntryFrame() const
    {
        return (GetFrameType() == FrameType::INTERPRETER_ENTRY_FRAME);
    }

    bool IsCInterpretedEntryFrame(FrameType type) const
    {
        return (type == FrameType::INTERPRETER_ENTRY_FRAME);
    }

    bool IsOptimizedEntryFrame(FrameType type) const
    {
        return type == FrameType::OPTIMIZED_ENTRY_FRAME;
    }

    bool IsJSEntryFrame(FrameType type) const
    {
        return IsAsmInterpretedEntryFrame(type) || IsOptimizedEntryFrame(type);
    }

    bool IsLeaveFrame() const
    {
        FrameType type = GetFrameType();
        return (type == FrameType::LEAVE_FRAME) || (type == FrameType::LEAVE_FRAME_WITH_ARGV);
    }

    bool IsInterpreterBuiltinFrame() const
    {
        FrameType type = GetFrameType();
        return type == FrameType::INTERPRETER_BUILTIN_FRAME;
    }

    bool IsBaselineBuiltinFrame(FrameType type) const
    {
        return (type == FrameType::BASELINE_BUILTIN_FRAME);
    }

    JSTaggedType *GetSp() const
    {
        return sp_;
    }

    JSTaggedType *GetFp() const
    {
        return fp_;
    }

    uintptr_t GetBaselineNativePc() const
    {
        return baselineNativePc_;
    }

    void PrevJSFrame();
    JSTaggedType *GetPrevJSFrame();

    // for InterpretedFrame.
    JSTaggedValue GetVRegValue(size_t index) const;
    void SetVRegValue(size_t index, JSTaggedValue value);

    // for BaselineBuiltinFrame
    void FindAndSetBaselineNativePc(FrameIterator it);

    JSTaggedValue GetEnv() const;
    JSTaggedValue GetAcc() const;
    uint32_t GetNumberArgs();
    uint32_t GetBytecodeOffset() const;
    Method *GetMethod() const;
    const JSPandaFile* GetJSPandaFile() const;
    std::string GetFileName() const;
    uint32_t GetAbcId() const;
    uint32_t GetMethodId() const;
    Method *CheckAndGetMethod() const;
    JSTaggedValue GetThis() const;
    JSTaggedValue GetFunction() const;
    const uint8_t *GetPc() const;
    ConstantPool *GetConstpool() const;

    void DumpStack(std::ostream &os) const;
    void DumpStack() const
    {
        DumpStack(std::cout);
    }

    void DumpPC(std::ostream &os, const uint8_t *pc) const;
    void DumpPC(const uint8_t *pc) const
    {
        DumpPC(std::cout, pc);
    }

    // for Frame GC.
    void Iterate(const RootVisitor &visitor, const RootRangeVisitor &rangeVisitor,
        const RootBaseAndDerivedVisitor &derivedVisitor);
    void IterateFrameChain(JSTaggedType *start, const RootVisitor &visitor, const RootRangeVisitor &rangeVisitor,
        const RootBaseAndDerivedVisitor &derivedVisitor) const;
    void IterateAssembleStack(const RootVisitor &visitor, const RootRangeVisitor &rangeVisitor,
        const RootBaseAndDerivedVisitor &derivedVisitor);
    void IterateEcmaRuntimeCallInfo(const RootVisitor &visitor, const RootRangeVisitor &rangeVisitor);

private:
    FrameType GetFrameType() const
    {
        ASSERT(HasFrame());
        FrameType *typeAddr = reinterpret_cast<FrameType *>(reinterpret_cast<uintptr_t>(sp_) - sizeof(FrameType));
        return *typeAddr;
    }

    void AdvanceToJSFrame();
    uintptr_t GetInterpretedFrameEnd(JSTaggedType *prevSp) const;
private:
    JSTaggedType *sp_ {nullptr};
    JSTaggedType *fp_ {nullptr};
    uintptr_t baselineNativePc_ {0}; // For baselineJit upFrame
    const JSThread *thread_ {nullptr};
    const kungfu::ArkStackMapParser *arkStackMapParser_ {nullptr};
};

class StackAssertScope {
public:
    explicit StackAssertScope(JSThread *thread) : thread_(thread), oldSp_(thread->GetCurrentSPFrame()) {}

    ~StackAssertScope()
    {
        if (!thread_->HasPendingException()) {
            DASSERT_PRINT(oldSp_ == thread_->GetCurrentSPFrame(),
                          "StackAssertScope assert failed, sp did not restore as expeted");
        }
    }

private:
    JSThread *thread_ {nullptr};
    const JSTaggedType *oldSp_ {nullptr};
};
}; // namespace ecmascript
}  // namespace panda
#endif  // ECMASCRIPT_INTERPRETER_FRAME_HANDLER_H
