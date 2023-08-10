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

#include "ecmascript/compiler/ts_class_analysis.h"
#include "ecmascript/ts_types/ts_type_accessor.h"

namespace panda::ecmascript::kungfu {
void TSClassAnalysis::Run() const
{
    const JSThread *thread = tsManager_->GetThread();
    std::set<GlobalTSTypeRef> &collectedGT = GetCollectedGT();
    for (auto iter = collectedGT.begin(); iter != collectedGT.end();) {
        JSHandle<JSTaggedValue> tsType = tsManager_->GetTSType(*iter);
        if (tsType->IsUndefined()) {
            ++iter;
            continue;
        }
        ASSERT(tsType->IsTSClassType());
        JSHandle<TSClassType> classType(tsType);
        if (CheckInitInfoOnInheritanceChain(*classType)) {
            AnalyzeProperties(thread, *classType);
            collectedGT.erase(iter++);
        } else {
            ++iter;
        }
    }
}

bool TSClassAnalysis::CheckInitInfoOnInheritanceChain(const TSClassType *classType) const
{
    DISALLOW_GARBAGE_COLLECTION;
    if (!tsManager_->HasTSHClass(classType)) {
        return false;
    }

    // All extended class types and itself have completed initialization analysis.
    if (classType->IsBaseClassType()) {
        return classType->GetHasAnalysedInitialization();
    }

    while (true) {
        if (!classType->GetHasAnalysedInitialization()) {
            return false;
        }

        if (classType->IsBaseClassType()) {
            break;
        }

        classType = tsManager_->GetExtendedClassType(classType);
    }
    return true;
}

bool TSClassAnalysis::HasEscapedThisOnSuper(const TSClassType *classType) const
{
    DISALLOW_GARBAGE_COLLECTION;
    if (classType->IsBaseClassType()) {
        return false;
    }

    while (true) {
        classType = tsManager_->GetExtendedClassType(classType);
        if (classType->GetHasEscapedThisInConstructor()) {
            return true;
        }

        if (classType->IsBaseClassType()) {
            break;
        }
    }
    return false;
}

void TSClassAnalysis::AnalyzeProperties(const JSThread *thread, const TSClassType *classType) const
{
    DISALLOW_GARBAGE_COLLECTION;
    // Using this in self constructor will not mark flag in initialization analysis.
    if (HasEscapedThisOnSuper(classType)) {
        return;
    }

    if (!classType->GetHasPassedSubtypingCheck()) {
        return;
    }

    GlobalTSTypeRef classGT = classType->GetGT();
    int hclassIndex = tsManager_->GetHClassIndex(classGT);
    ASSERT(hclassIndex != -1);
    JSHClass *hclass = JSHClass::Cast(tsManager_->GetValueFromCache(hclassIndex).GetTaggedObject());
    if (UNLIKELY(hclass->IsDictionaryMode())) {
        return;
    }

    LayoutInfo *layout = LayoutInfo::Cast(hclass->GetLayout().GetTaggedObject());

    for (uint32_t index = 0; index < hclass->NumberOfProps(); ++index) {
        JSTaggedValue key = layout->GetKey(index);
        if (AnalyzePropertyOnSelf(thread, classType, key) || AnalyzePropertyOnSupers(thread, classType, key)) {
            layout->SetIsNotHole(thread, index);
        }
    }
}

bool TSClassAnalysis::AnalyzePropertyOnSelf(const JSThread *thread, const TSClassType *classType,
                                            JSTaggedValue key) const
{
    DISALLOW_GARBAGE_COLLECTION;
    JSHandle<TSObjectType> instanceType(thread, classType->GetInstanceType());
    JSHandle<TSObjLayoutInfo> tsLayout(thread, instanceType->GetObjLayoutInfo());
    int index = tsLayout->GetElementIndexByKey(key);
    if (!TSObjLayoutInfo::IsValidIndex(index)) {
        return false;
    }
    TSFieldAttributes tsAttr(tsLayout->GetAttribute(index).GetInt());
    return tsAttr.GetInitializedFlag();
}

bool TSClassAnalysis::AnalyzePropertyOnSupers(const JSThread *thread, const TSClassType *classType,
                                              JSTaggedValue key) const
{
    DISALLOW_GARBAGE_COLLECTION;
    if (classType->IsBaseClassType()) {
        return false;
    }

    TSClassType *type = tsManager_->GetExtendedClassType(classType);
    while (true) {
        if (AnalyzePropertyOnSelf(thread, type, key)) {
            return true;
        }

        if (type->IsBaseClassType()) {
            break;
        }

        type = tsManager_->GetExtendedClassType(type);
    }
    return false;
}
}  // namespace panda::ecmascript::kungfu
