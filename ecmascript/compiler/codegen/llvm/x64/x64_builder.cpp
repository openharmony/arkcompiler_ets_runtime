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

class X64TargetBuilder final : public LLVMTargetBuilder {
public:
    ~X64TargetBuilder() override = default;

    LLVMValueRef GetASMBarrierCall(LLVMModule* llvmModule) override
    {
        std::string asmCall = "call *${0:c}"; // call to the first input register.
        std::string constraints = "r,{rdi},{rsi},{rdx},{rcx},"
        // input registerds, first is the runtime check barrier stub.
        // others are same with the sign of runtime check barrier stub.
        "~{r11},~{dirflag},~{fpsr},~{flags},"
        // r11 will be used as scratch register, so mark it as clobbered, all the flag registers are also clobbered.
        "~{xmm0},~{xmm1},~{xmm2},~{xmm3},~{xmm4},~{xmm5},~{xmm6},~{xmm7},"
        "~{xmm8},~{xmm9},~{xmm10},~{xmm11},~{xmm12},~{xmm13},~{xmm14},~{xmm15}";
        // can't promise the vector registers are preserved, so mark them clobbered.
        // NOTE: if AVX512 or more vector registers are enabled, need add them to clobber list.
        const CallSignature* cs = RuntimeStubCSigns::Get(RuntimeStubCSigns::ID_ASMFastWriteBarrier);
        std::vector<LLVMTypeRef> paramTys;
        paramTys.push_back(llvmModule->GetRawPtrT()); // add the runtime check barrier stub as the first arg.
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

class X64TargetBuilderRegistry {
public:
    X64TargetBuilderRegistry()
    {
        LLVMIRBuilder::RegisterTargetBuilder(TARGET_X64, []()-> LLVMTargetBuilder* {
            return new X64TargetBuilder();
        });
    }
};

X64TargetBuilderRegistry g_x64Registry;
}
