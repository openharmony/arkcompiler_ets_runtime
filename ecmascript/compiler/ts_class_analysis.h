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

#ifndef ECMASCRIPT_COMPILER_TS_CLASS_ANALYSIS_H
#define ECMASCRIPT_COMPILER_TS_CLASS_ANALYSIS_H

#include "ecmascript/ts_types/ts_manager.h"

namespace panda::ecmascript::kungfu {
class TSClassAnalysis {
public:
    TSClassAnalysis(TSManager *tsManager): tsManager_(tsManager) {}
    ~TSClassAnalysis() = default;

    void Run() const;

private:
    bool CheckInitInfoOnInheritanceChain(const TSClassType *classType) const;

    bool HasEscapedThisOnSuper(const TSClassType *classType) const;

    void AnalyzeProperties(const JSThread *thread, const TSClassType *classType) const;

    bool AnalyzePropertyOnSelf(const JSThread *thread, const TSClassType *classType, JSTaggedValue key) const;

    bool AnalyzePropertyOnSupers(const JSThread *thread, const TSClassType *classType, JSTaggedValue key) const;

    inline std::set<GlobalTSTypeRef>& GetCollectedGT() const
    {
        return tsManager_->GetCollectedGT();
    }

    TSManager *tsManager_ {nullptr};
};
}  // namespace panda::ecmascript::kungfu
#endif  // ECMASCRIPT_COMPILER_TS_CLASS_ANALYSIS_H
