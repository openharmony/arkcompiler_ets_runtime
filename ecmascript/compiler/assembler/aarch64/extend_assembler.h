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

#ifndef ECMASCRIPT_COMPILER_AARCH64_EXTEND_ASSEMBLER_H
#define ECMASCRIPT_COMPILER_AARCH64_EXTEND_ASSEMBLER_H

#include "ecmascript/compiler/assembler/aarch64/assembler_aarch64.h"
#include "ecmascript/compiler/assembler_module.h"
#include "ecmascript/compiler/bc_call_signature.h"

namespace panda::ecmascript::aarch64 {
using namespace panda::ecmascript::kungfu;
// ExtendAssembler implements frequently-used assembler macros.
class ExtendedAssembler : public AssemblerAarch64 {
public:
    static constexpr int FRAME_SLOT_SIZE = 8;
    static constexpr int PAIR_SLOT_SIZE = 16;
    ExtendedAssembler(Chunk *chunk, AssemblerModule *module)
        : AssemblerAarch64(chunk), module_(module)
    {
    }
    void BindAssemblerStub(int id);
    void CalleeSave();
    void CalleeRestore();
    void CallAssemblerStub(int id, bool isTail = false);
    void PushFpAndLr();
    void SaveFpAndLr();
    void RestoreFpAndLr();
    void PushLrAndFp();
    void SaveLrAndFp();
    void RestoreLrAndFp();
    void PushArgc(int32_t argc, Register op, Register fp);
    void PushArgc(Register argc, Register op, Register fp);
    void Align16(Register fp);

    Register TempRegister1()
    {
        if (temp1InUse_) {
            LOG_COMPILER(FATAL) << "temp register1 inuse.";
            UNREACHABLE();
        }
        temp1InUse_ = true;
        return X9;
    }
    Register TempRegister2()
    {
        if (temp2InUse_) {
            LOG_COMPILER(FATAL) << "temp register2 inuse.";
            UNREACHABLE();
        }
        temp2InUse_ = true;
        return X10;
    }
    Register AvailableRegister1() const
    {
        // X11 is neither callee saved reegister nor argument register
        return X11;
    }
    Register AvailableRegister2() const
    {
        // X12 is neither callee saved reegister nor argument register
        return X12;
    }
    Register AvailableRegister3() const
    {
        // X13 is neither callee saved reegister nor argument register
        return X13;
    }
    Register AvailableRegister4() const
    {
        // X14 is neither callee saved reegister nor argument register
        return X14;
    }
    Register CallDispatcherArgument(kungfu::CallDispatchInputs index)
    {
        size_t i = static_cast<size_t>(index);
        Register ret = isGhcCallingConv_ ? ghcJSCallDispacherArgs_[i] : cppJSCallDispacherArgs_[i];
        if (ret.GetId() == INVALID_REG) {
            LOG_COMPILER(FATAL) << "arm64 invalid call argument:" << i;
        }
        return ret;
    }
    Register GlueRegister()
    {
        return isGhcCallingConv_ ? X19 : X0;
    }

    bool FromInterpreterHandler() const
    {
        return isGhcCallingConv_;
    }
private:
    AssemblerModule *module_ {nullptr};
    bool isGhcCallingConv_ {false};
    bool temp1InUse_ {false};
    bool temp2InUse_ {false};
    friend class TempRegister1Scope;
    friend class TempRegister2Scope;

    static constexpr size_t JS_CALL_DISPATCHER_ARGS_COUNT =
        static_cast<size_t>(kungfu::CallDispatchInputs::NUM_OF_INPUTS);
    static Register ghcJSCallDispacherArgs_[JS_CALL_DISPATCHER_ARGS_COUNT];
    static Register cppJSCallDispacherArgs_[JS_CALL_DISPATCHER_ARGS_COUNT];
};

class TempRegister1Scope {
public:
    explicit TempRegister1Scope(ExtendedAssembler *assembler) : assembler_(assembler) {}
    ~TempRegister1Scope()
    {
        assembler_->temp1InUse_ = false;
    }

    NO_COPY_SEMANTIC(TempRegister1Scope);
    NO_MOVE_SEMANTIC(TempRegister1Scope);
private:
    ExtendedAssembler *assembler_;
};

class TempRegister2Scope {
public:
    explicit TempRegister2Scope(ExtendedAssembler *assembler) : assembler_(assembler) {}
    ~TempRegister2Scope()
    {
        assembler_->temp2InUse_ = false;
    }

    NO_COPY_SEMANTIC(TempRegister2Scope);
    NO_MOVE_SEMANTIC(TempRegister2Scope);
private:
    ExtendedAssembler *assembler_;
};
}  // panda::ecmascript::aarch64
#endif  // ECMASCRIPT_COMPILER_AARCH64_EXTEND_ASSEMBLER_H