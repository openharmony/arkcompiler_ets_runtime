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

#include "ecmascript/ecma_vm.h"

#include "libpandafile/bytecode_instruction-inl.h"

namespace panda::ecmascript::kungfu {
struct CfgInfo;
class BytecodeInfoCollector {
public:
    // need to remove in the future
    enum FixInsIndex : uint8_t { FIX_ONE = 1, FIX_TWO = 2, FIX_FOUR = 4 };

    struct MethodPcInfo {
        const JSMethod *method {nullptr};
        std::map<uint8_t *, uint8_t *> byteCodeCurPrePc {};
        std::vector<CfgInfo> bytecodeBlockInfos {};
        std::map<const uint8_t *, int32_t> pcToBCOffset {};
    };

    struct BCInfo {
        const JSPandaFile *jsPandaFile {nullptr};
        JSHandle<JSTaggedValue> constantPool;
        std::vector<MethodPcInfo> methodPcInfos {};

        template <class Callback>
        void EnumerateBCInfo(const Callback &cb)
        {
            for (size_t i = 0; i < methodPcInfos.size(); i++) {
                cb(jsPandaFile, constantPool, methodPcInfos[i]);
            }
        }
    };

    static const JSPandaFile *LoadInfoFromPf(const CString &filename, const std::string_view& entryPoint,
                                             std::vector<MethodPcInfo> &methodPcInfos);

private:
    static void ProcessClasses(JSPandaFile *jsPandaFile, const CString &methodName,
                               std::vector<MethodPcInfo> &methodPcInfos);

    // need to remove in the future
    static void FixOpcode(uint8_t *pc);

    // need to remove in the future
    static void UpdateICOffset(JSMethod *method, uint8_t *pc);

    // need to remove in the future
    static void FixInstructionId32(const BytecodeInstruction &inst, uint32_t index, uint32_t fixOrder = 0);

    // need to remove in the future
    static void TranslateBCIns(JSPandaFile *jsPandaFile, const BytecodeInstruction &bcIns, const JSMethod *method);

    static void CollectMethodPcs(JSPandaFile *jsPandaFile, const uint32_t insSz, const uint8_t *insArr,
                                 const JSMethod *method, std::vector<MethodPcInfo> &methodPcInfos);
};
}  // namespace panda::ecmascript::kungfu
#endif  // ECMASCRIPT_COMPILER_BYTECODE_INFO_COLLECTOR_H