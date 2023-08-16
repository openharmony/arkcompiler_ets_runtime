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

#ifndef ECMASCRIPT_COMPILER_TYPE_RECORDER_H
#define ECMASCRIPT_COMPILER_TYPE_RECORDER_H

#include "ecmascript/compiler/type.h"
#include "ecmascript/jspandafile/js_pandafile.h"
#include "ecmascript/method.h"
#include "ecmascript/pgo_profiler/pgo_profiler_decoder.h"
#include "ecmascript/ts_types/ts_manager.h"

namespace panda::ecmascript::kungfu {
enum class TypedArgIdx : uint8_t {
    FUNC = 0,
    NEW_TARGET,
    THIS_OBJECT,
    NUM_OF_TYPED_ARGS,
};

class TypeRecorder {
public:
    TypeRecorder(const JSPandaFile *jsPandaFile, const MethodLiteral *methodLiteral,
                 TSManager *tsManager, const CString &recordName, PGOProfilerDecoder *decoder,
                 const MethodPcInfo &methodPCInfo, const Bytecodes *bytecodes, bool enableOptTrackField);
    ~TypeRecorder() = default;

    GateType GetType(const int32_t offset) const;
    ElementsKind GetElementsKind(PGOSampleType type) const;
    PGOSampleType GetOrUpdatePGOType(TSManager *tsManager, int32_t offset, const GateType &type) const;
    PGORWOpType GetRwOpType(int32_t offset) const;
    ElementsKind GetElementsKind(int32_t offset) const;
    GateType GetArgType(const uint32_t argIndex) const;
    GateType UpdateType(const int32_t offset, const GateType &type) const;
    GateType GetCallTargetType(int32_t offset) const;

    static constexpr int METHOD_ANNOTATION_THIS_TYPE_OFFSET = -2;

private:
    void LoadTypes(const JSPandaFile *jsPandaFile, const MethodLiteral *methodLiteral,
                   TSManager *tsManager, const CString &recordName);

    void CreateTypesForPGO(const JSPandaFile *jsPandaFile, const MethodLiteral *methodLiteral,
                           TSManager *tsManager, const CString &recordName);

    void LoadTypesFromPGO(const JSPandaFile *jsPandaFile, const MethodLiteral *methodLiteral,
                          const CString &recordName);

    void LoadArgTypes(const TSManager *tsManager, GlobalTSTypeRef funcGT, GlobalTSTypeRef thisGT);

    // This function tries to get the type of 'this'. In success, the type of the class is returned if the function is
    // a static member, or the type of instances of the class is returned. Otherwise the type 'any' is returned.
    GateType TryGetThisType(const TSManager *tsManager, GlobalTSTypeRef funcGT, GlobalTSTypeRef thisGT) const;

    // This function tries to get the type of 'newTarget'. In success, the type of the class is returned. Otherwise,
    // the type 'any' is returned.
    GateType TryGetNewTargetType(const TSManager *tsManager, GlobalTSTypeRef thisGT) const;

    // This function tries to get the type of the function. On failure, the type 'any' is returned.
    GateType TryGetFuncType(GlobalTSTypeRef funcGT) const;

    bool TypeNeedFilter(GlobalTSTypeRef gt) const;

    bool CheckTypeMarkForDefineFunc(uint32_t checkBc) const;

    void CollectLiteralGT(TSManager *tsManager, TypeLocation &tLoc, GlobalTSTypeRef gt);

    std::unordered_map<int32_t, GateType> bcOffsetGtMap_ {};
    std::unordered_map<int32_t, GateType> bcOffsetCallTargetGtMap_ {};
    std::unordered_map<int32_t, PGOSampleType> bcOffsetPGOOpTypeMap_ {};
    std::unordered_map<int32_t, PGORWOpType> bcOffsetPGORwTypeMap_ {};
    std::vector<GateType> argTypes_;
    PGOProfilerDecoder *decoder_ {nullptr};
    bool enableOptTrackField_ {false};
    const std::vector<const uint8_t*> &pcOffsets_;
    const Bytecodes *bytecodes_ {nullptr};
};
}  // panda::ecmascript::kungfu
#endif  // ECMASCRIPT_COMPILER_TYPE_RECORDER_H
