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

#ifndef ECMASCRIPT_COMPILER_TYPE_INFERENCE_GLOBAL_TYPE_INFER_H
#define ECMASCRIPT_COMPILER_TYPE_INFERENCE_GLOBAL_TYPE_INFER_H

#include "ecmascript/compiler/type_inference/method_type_infer.h"

namespace panda::ecmascript::kungfu {
class GlobalTypeInfer {
public:
    GlobalTypeInfer(PassContext *ctx, const uint32_t methodOffset, const CString &recordName,
                    PGOProfilerDecoder *decoder, bool enableOptTrackField, bool enableLog);
    ~GlobalTypeInfer();

    void ProcessTypeInference(BytecodeCircuitBuilder *builder, Circuit *circuit);

private:
    static constexpr size_t MAX_GLOBAL_INFER_ALLOWED = 2;

    inline MethodTypeInfer *GetTypeInfer(const uint32_t methodOffset) const
    {
        auto it = typeInfers_.find(methodOffset);
        if (it != typeInfers_.end()) {
            return it->second;
        }
        return nullptr;
    }

    inline bool IsLegalMethod(const uint32_t methodOffset) const
    {
        return (methodOffset != 0);
    }

    void NewTypeInfer(const uint32_t methodOffset);
    void CollectNamespaceMethod(const uint32_t methodOffset);
    void CollectNamespaceMethods(const uint32_t methodOffset);
    void RunTypeCheck();

    PassContext *ctx_ {nullptr};
    const JSPandaFile *jsPandaFile_ {nullptr};
    BCInfo &bcInfo_;
    uint32_t methodOffset_ {0};
    const CString &recordName_;
    PGOProfilerDecoder *decoder_ {nullptr};
    bool enableOptTrackField_ {false};
    bool enableLog_ {false};
    bool enableGlobalTypeInfer_ {false};
    std::set<uint32_t> namespaceTypes_ {};
    std::vector<BytecodeCircuitBuilder *> builders_ {};
    std::vector<Circuit *> circuits_ {};
    std::map<uint32_t, MethodTypeInfer *> typeInfers_ {};
};
}  // namespace panda::ecmascript::kungfu
#endif  // ECMASCRIPT_COMPILER_TYPE_INFERENCE_GLOBAL_TYPE_INFER_H
