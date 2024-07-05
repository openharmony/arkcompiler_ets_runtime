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
    PGOHClassGenerator(const PGOTypeRecorder &typeRecorder, PGOTypeManager *ptManager)
        : typeRecorder_(typeRecorder), ptManager_(ptManager) {}

    bool FindHClassLayoutDesc(PGOSampleType type) const;
    // The hclass of object literal needs to be created from cached
    bool GenerateHClass(PGOSampleType type, bool isCache) const;
    bool GenerateIHClass(PGOSampleType type, const JSHandle<JSObject> &prototype) const;

private:
    void CaculateMaxNumOfObj(
        const PGOHClassTreeDesc *desc, const HClassLayoutDesc *parent, uint32_t lastNum, uint32_t &maxNum) const;
    bool CheckIsValid(PGOSampleType type, uint32_t maxNum, bool isCache) const;
    void CaculateMaxNumOfObjIncludeProtoTransition(PGOSampleType type, uint32_t &maxNum) const;
    JSHandle<JSHClass> CreateRootPHClass(
        ProfileType rootType, const HClassLayoutDesc *layoutDesc, uint32_t maxNum, bool isTransitionPhc) const;
    JSHandle<JSHClass> CreateRootCHClass(
        ProfileType rootType, const HClassLayoutDesc *layoutDesc, uint32_t maxNum) const;
    JSHandle<JSHClass> CreateRootHClass(
        ProfileType rootType, const HClassLayoutDesc *layoutDesc, uint32_t maxNum, bool isCache) const;

    void CreateChildHClass(ProfileType rootType, const PGOHClassTreeDesc *desc, const JSHandle<JSHClass> &parent,
        const HClassLayoutDesc *parentLayoutDesc) const;

    const PGOTypeRecorder &typeRecorder_;
    PGOTypeManager *ptManager_;
};
}  // panda::ecmascript::kungfu
#endif // ECMASCRIPT_COMPILER_PGO_TYPE_PGO_HCLASS_GENERATOR_H
