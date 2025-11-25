/*
 * Copyright (c) 2021 Huawei Device Co., Ltd.
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

#include "ecmascript/mem/local_cmc/cc_marker-inl.h"

#include "ecmascript/js_hclass-inl.h"
#include "ecmascript/mem/object_xray.h"


namespace panda::ecmascript {
void CCMarker::ProcessMarkStack(uint32_t threadId)
{
    WorkNodeHolder *workNodeHolder = workManager_->GetWorkNodeHolder(threadId);
    CCMarkObjectVisitor ccMarkObjectVisitor(workNodeHolder);
    SemiSpace *newSpace = heap_->GetNewSpace();
    TaggedObject *obj = nullptr;
    while (workNodeHolder->Pop(&obj)) {
        Region *region = Region::ObjectAddressToRange(obj);
        if (region->IsHalfFreshRegion()) {
            ASSERT(region->InYoungSpace());
            if (newSpace->IsFreshObjectInHalfFreshRegion(obj)) {
                // Fresh object do not need to visit body.
                continue;
            }
        }
        auto jsHClass = obj->GetClass();
        auto size = jsHClass->SizeFromJSHClass(obj);
        region->IncreaseAliveObject(size);
        ccMarkObjectVisitor.VisitObjectHClassImpl(jsHClass);
        ObjectXRay::VisitObjectBody<VisitType::OLD_GC_VISIT>(obj, jsHClass, ccMarkObjectVisitor);
    }
}

void CCMarker::MarkJitCodeMap(uint32_t threadId)
{
    // To keep MachineCode objects alive (for dump) before JsError object be free, we have to know which JsError is
    // alive first. So this method must be call after all other mark work finish.
    ECMA_BYTRACE_NAME(HITRACE_LEVEL_COMMERCIAL, HITRACE_TAG_ARK, "GC::MarkJitCodeMap", "");
    ASSERT(heap_->IsCCMark());
    CCMarkObjectVisitor objectVisitor(workManager_->GetWorkNodeHolder(threadId));
    JitCodeMapVisitor visitor = [&objectVisitor](std::map<JSTaggedType, JitCodeVector *> &jitCodeMaps) {
        auto it = jitCodeMaps.begin();
        while (it != jitCodeMaps.end()) {
            JSTaggedType jsError = it->first;
            Region *objectRegion = Region::ObjectAddressToRange(reinterpret_cast<TaggedObject *>(jsError));
            if (!objectRegion->Test(reinterpret_cast<TaggedObject *>(jsError))) {
                ++it;
                continue;
            }
            for (auto &jitCodeMap : *(it->second)) {
                auto &jitCode = std::get<0>(jitCodeMap);
                objectVisitor.HandleObject(jitCode);
            }
            ++it;
        }
    };
    ObjectXRay::VisitJitCodeMap(heap_->GetEcmaVM(), visitor);
    ProcessMarkStack(threadId);
    heap_->WaitRunningTaskFinished();
}

}  // namespace panda::ecmascript
