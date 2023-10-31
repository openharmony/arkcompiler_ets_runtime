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

#include "ecmascript/compiler/pgo_type/pgo_type_recorder.h"

namespace panda::ecmascript::kungfu {
using PGOType = pgo::PGOType;

PGOTypeRecorder::PGOTypeRecorder(
    const PGOProfilerDecoder &decoder, const JSPandaFile *jsPandaFile, uint32_t methodOffset)
    : decoder_(decoder)
{
    auto callback = [this] (uint32_t offset, const PGOType *type) {
        if (type->IsScalarOpType()) {
            bcOffsetPGOOpTypeMap_.emplace(offset, reinterpret_cast<const PGOSampleType *>(type));
        } else if (type->IsDefineOpType()) {
            bcOffsetPGODefOpTypeMap_.emplace(offset, reinterpret_cast<const PGODefineOpType *>(type));
        }
    };
    const CString recordName = MethodLiteral::GetRecordName(jsPandaFile, EntityId(methodOffset));
    auto methodLiteral = jsPandaFile->FindMethodLiteral(methodOffset);
    decoder.GetTypeInfo(jsPandaFile, recordName, methodLiteral, callback);
}
}  // namespace panda::ecmascript
