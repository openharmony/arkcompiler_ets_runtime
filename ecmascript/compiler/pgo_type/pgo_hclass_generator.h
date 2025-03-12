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
#ifndef ECMASCRIPT_COMPILER_PGO_TYPE_PGO_HCLASS_GENERATOR_H
#define ECMASCRIPT_COMPILER_PGO_TYPE_PGO_HCLASS_GENERATOR_H

#include "ecmascript/compiler/pgo_type/pgo_type_manager.h"
#include "ecmascript/compiler/pgo_type/pgo_type_recorder.h"
#include "ecmascript/js_hclass.h"

namespace panda::ecmascript::kungfu {
class PGOHClassGenerator {
public:
    enum class Status: uint8_t {
        NONE = 0x00UL,
        ISCACHE = 0x01UL,
    };

    PGOHClassGenerator(const PGOTypeRecorder &typeRecorder, PGOTypeManager *ptManager, Status status = Status::NONE)
        : typeRecorder_(typeRecorder), ptManager_(ptManager), status_(status) {}

    bool FindHClassLayoutDesc(PGOSampleType type) const;
    bool GenerateHClass(PGOSampleType type) const;
    bool GenerateIHClass(PGOSampleType type, const JSHandle<JSTaggedValue> &prototype) const;

    bool IsCache() const;
    void SetStatus(Status status) const;

private:
    void CalculateMaxNumOfObj(
        const PGOHClassTreeDesc *desc, const HClassLayoutDesc *parent, uint32_t lastNum, uint32_t &maxNum) const;
    bool CheckIsValid(PGOSampleType type, uint32_t maxNum) const;
    void CalculateMaxNumOfObjIncludeProtoTransition(PGOSampleType type, uint32_t &maxNum) const;
    JSHandle<JSHClass> CreateRootPHClass(
        ProfileType rootType, const HClassLayoutDesc *layoutDesc, uint32_t maxNum, bool isTransitionPhc) const;
    JSHandle<JSHClass> CreateRootCHClass(
        ProfileType rootType, const HClassLayoutDesc *layoutDesc, uint32_t maxNum) const;
    JSHandle<JSHClass> CreateRootHClass(
        ProfileType rootType, const HClassLayoutDesc *layoutDesc, uint32_t maxNum) const;
    JSHandle<JSHClass> CreateRootHClassWithCached(
        ProfileType rootType, const HClassLayoutDesc *layoutDesc, uint32_t literalLength, uint32_t maxPropsNum) const;

    void CreateChildHClass(ProfileType rootType, const PGOHClassTreeDesc *desc, const JSHandle<JSHClass> &parent,
        const HClassLayoutDesc *parentLayoutDesc) const;

    const PGOTypeRecorder &typeRecorder_;
    PGOTypeManager *ptManager_;
    mutable Status status_;
};
}  // panda::ecmascript::kungfu
#endif // ECMASCRIPT_COMPILER_PGO_TYPE_PGO_HCLASS_GENERATOR_H
