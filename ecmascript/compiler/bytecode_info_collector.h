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

#ifndef ECMASCRIPT_COMPILER_BYTECODE_INFO_COLLECTOR_H
#define ECMASCRIPT_COMPILER_BYTECODE_INFO_COLLECTOR_H

#include "ecmascript/jspandafile/js_pandafile.h"
#include "ecmascript/jspandafile/method_literal.h"
#include "ecmascript/jspandafile/bytecode_inst/old_instruction.h"
#include "libpandafile/bytecode_instruction-inl.h"

namespace panda::ecmascript::kungfu {
struct CfgInfo;

// need to remove in the future
enum FixInsIndex : uint8_t { FIX_ONE = 1, FIX_TWO = 2, FIX_FOUR = 4 };

/*    ts source code
 *    function f() {
 *        function g() {
 *            return 0;
 *        }
 *        return g();
 *    }
 *
 *                                     The structure of Lexical Environment
 *
 *                                               Lexical Environment             Lexical Environment
 *               Global Environment                 of function f                   of function g
 *              +-------------------+ <----+    +-------------------+ <----+    +-------------------+
 *    null <----|  Outer Reference  |      +----|  Outer Reference  |      +----|  Outer Reference  |
 *              +-------------------+           +-------------------+           +-------------------+
 *              |Environment Recoder|           |Environment Recoder|           |Environment Recoder|
 *              +-------------------+           +-------------------+           +-------------------+
 *
 *    We only record the type of the variable in Environment Recoder.
 */

struct LexEnv {
    uint32_t outmethodId {0};
    std::vector<GateType> lexVarTypes {};
};

// each method in the abc file corresponds to one MethodInfo and
// methods with the same instructions share one common MethodPcInfo
struct MethodPcInfo {
    std::map<uint8_t *, uint8_t *> byteCodeCurPrePc {};
    std::vector<CfgInfo> bytecodeBlockInfos {};
    std::map<const uint8_t *, int32_t> pcToBCOffset {};
    uint32_t methodsSize {0};
};

struct MethodInfo {
    // used to record the index of the current MethodInfo to speed up the lookup of lexEnv
    size_t methodInfoIndex;
    // used to obtain MethodPcInfo from the vector methodPcInfos of struct BCInfo
    size_t methodPcInfoIndex;
    std::vector<uint32_t> innerMethods;
    LexEnv lexEnv;
};

struct BCInfo {
    std::vector<uint32_t> mainMethodIndexes {};
    std::vector<CString> recordNames {};
    std::vector<MethodPcInfo> methodPcInfos {};
    std::unordered_map<uint32_t, MethodInfo> methodList {};

    template <class Callback>
    void EnumerateBCInfo(const Callback &cb)
    {
        for (uint32_t i = 0; i < mainMethodIndexes.size(); i++) {
            std::queue<uint32_t> methodCompiledOrder;
            methodCompiledOrder.push(mainMethodIndexes[i]);
            while (!methodCompiledOrder.empty()) {
                auto compilingMethod = methodCompiledOrder.front();
                methodCompiledOrder.pop();
                auto &methodInfo = methodList.at(compilingMethod);
                auto &methodPcInfo = methodPcInfos[methodInfo.methodPcInfoIndex];
                cb(recordNames[i], compilingMethod, methodPcInfo, methodInfo.methodInfoIndex);
                auto &innerMethods = methodInfo.innerMethods;
                for (auto it : innerMethods) {
                    methodCompiledOrder.push(it);
                }
            }
        }
    }
};

class LexEnvManager {
public:
    explicit LexEnvManager(BCInfo &bcInfo);
    ~LexEnvManager() = default;
    NO_COPY_SEMANTIC(LexEnvManager);
    NO_MOVE_SEMANTIC(LexEnvManager);

    void SetLexEnvElementType(uint32_t methodId, uint32_t level, uint32_t slot, const GateType &type);
    GateType GetLexEnvElementType(uint32_t methodId, uint32_t level, uint32_t slot) const;

private:
    std::vector<LexEnv *> lexEnvs_ {};
};

class BytecodeInfoCollector {
public:
    explicit BytecodeInfoCollector(JSPandaFile *jsPandaFile) : jsPandaFile_(jsPandaFile)
    {
        ProcessClasses();
    }
    ~BytecodeInfoCollector() = default;
    NO_COPY_SEMANTIC(BytecodeInfoCollector);
    NO_MOVE_SEMANTIC(BytecodeInfoCollector);

    const JSPandaFile *GetJsPandaFile() const
    {
        return jsPandaFile_;
    }

    BCInfo &GetBytecodeInfo()
    {
        return bytecodeInfo_;
    }

private:
    inline size_t GetMethodInfoID()
    {
        return methodInfoIndex_++;
    }

    const CString GetEntryFunName(const std::string_view &entryPoint) const;
    void ProcessClasses();
    void CollectMethodPcs(const uint32_t insSz, const uint8_t *insArr, const MethodLiteral *method,
                          const CString &entryPoint = "func_main_0");
    void CollectMethodPcsFromNewBc(const uint32_t insSz, const uint8_t *insArr, const MethodLiteral *method);
    void SetMethodPcInfoIndex(uint32_t methodOffset, size_t index);
    void CollectInnerMethods(const MethodLiteral *method, uint32_t innerMethodOffset);
    void CollectInnerMethods(uint32_t methodId, uint32_t innerMethodOffset);
    void CollectInnerMethodsFromLiteral(const MethodLiteral *method, uint64_t index);
    void NewLexEnvWithSize(const MethodLiteral *method, uint64_t numOfLexVars);

    static void AddNopInst(uint8_t *pc, int number);

    // need to remove in the future
    static void FixOpcode(MethodLiteral *method, const OldBytecodeInst &inst);
    static void FixOpcode(const OldBytecodeInst &inst);

    // need to remove in the future
    static void UpdateICOffset(MethodLiteral *method, uint8_t *pc);

    // need to remove in the future
    static void FixInstructionId32(const OldBytecodeInst &inst, uint32_t index, uint32_t fixOrder = 0);

    // need to remove in the future
    void TranslateBCIns(const OldBytecodeInst &bcIns, const MethodLiteral *method, const CString &entryPoint);

    // use for new ISA
    void CollectInnerMethodsFromNewLiteral(const MethodLiteral *method, panda_file::File::EntityId literalId);
    void CollectMethodInfoFromNewBC(const BytecodeInstruction &bcIns, const MethodLiteral *method);

    JSPandaFile *jsPandaFile_ {nullptr};
    BCInfo bytecodeInfo_;
    size_t methodInfoIndex_ {0};
};
}  // namespace panda::ecmascript::kungfu
#endif  // ECMASCRIPT_COMPILER_BYTECODE_INFO_COLLECTOR_H
