/*
* Copyright (c) 2024 Huawei Device Co., Ltd.
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

#include "ecmascript/compiler/codegen/llvm/llvm_ir_builder.h"


namespace panda::ecmascript::kungfu {

class Aarch64TargetBuilder final : public LLVMTargetBuilder {
public:
    ~Aarch64TargetBuilder() override = default;

    LLVMValueRef GetASMBarrierCall(LLVMModule* llvmModule, bool isDirectCall) override
    {
        std::string asmCall;
        std::string inputRegs;
        if (isDirectCall) {
            asmCall = "bl " + RuntimeStubCSigns::GetRTName(RuntimeStubCSigns::ID_ASMFastWriteBarrier);
            inputRegs = "{x0},{x1},{x2},{x3},";
            // input registers are same with the sign of runtime check barrier stub.
        } else {
            asmCall = "blr $0"; // call to the first input register.
            inputRegs = "r,{x0},{x1},{x2},{x3},";
            // input registers, first is the runtime check barrier stub.
            // others are same with the sign of runtime check barrier stub.
        }
        std::string constraints = inputRegs + "~{x15},~{nzcv},~{fpsr},~{x30}"
            // x15 will be used as scratch register, so mark it as clobbered, all the flag registers are also clobbered.
            // lr will be early clobbered at call.
            "~{q0},~{q1},~{q2},~{q3},~{q4},~{q5},~{q6},~{q7},"
            // d8 ~ d15 are callee saved in C calling conv, do not mark q8 ~ q15 clobbered, but we can't use q8 ~ q15
            // cross such call site
            "~{q16},~{q17},~{q18},~{q19},~{q20},~{q21},~{q22},~{q23},~{q24},~{q25},~{q26},~{q27},~{q28},~{q29},~{q30},"
            "~{q31}";
        const CallSignature* cs = RuntimeStubCSigns::Get(RuntimeStubCSigns::ID_ASMFastWriteBarrier);
        std::vector<LLVMTypeRef> paramTys;
        if (!isDirectCall) {
            paramTys.push_back(llvmModule->GetRawPtrT()); // add the runtime check barrier stub as the first arg.
        }
        const size_t count = cs->GetParametersCount();
        const VariableType* originParamType = cs->GetParametersType();
        for (size_t i = 0; i < count; i++) {
            paramTys.push_back(llvmModule->ConvertLLVMTypeFromVariableType(originParamType[i]));
        }
        LLVMTypeRef functype = LLVMFunctionType(llvmModule->GetVoidT(), paramTys.data(), paramTys.size(), false);
#if defined(PANDA_TARGET_MACOS)
        return LLVMGetInlineAsm(functype, asmCall.data(), asmCall.size(), constraints.data(),
                                constraints.size(), true, true, LLVMInlineAsmDialectATT);
#else
        return LLVMGetInlineAsm(functype, asmCall.data(), asmCall.size(), constraints.data(),
                                constraints.size(), true, true, LLVMInlineAsmDialectATT, false);
#endif
    }
};

class Aarch64TargetBuilderRegistry {
public:
    Aarch64TargetBuilderRegistry()
    {
        LLVMIRBuilder::RegisterTargetBuilder(TARGET_AARCH64, []()-> LLVMTargetBuilder* {
            return new Aarch64TargetBuilder();
        });
    }
};

Aarch64TargetBuilderRegistry g_aarch64Registry;
}
