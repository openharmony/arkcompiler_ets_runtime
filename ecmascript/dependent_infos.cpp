/*
 * Copyright (c) 2025 Huawei Device Co., Ltd.
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

#include "ecmascript/dependent_infos.h"

#include "ecmascript/deoptimizer/deoptimizer.h"
#include "ecmascript/js_function.h"
#include "ecmascript/js_tagged_value.h"
#include "ecmascript/dfx/stackinfo/js_stackinfo.h"

namespace panda::ecmascript {

JSHandle<DependentInfos> DependentInfos::AppendDependentInfos(JSThread *thread,
                                                              const JSHandle<JSTaggedValue> jsFunc,
                                                              const DependentGroups groups,
                                                              const JSHandle<DependentInfos> info)
{
    JSHandle<JSTaggedValue> first = jsFunc;
    JSHandle<JSTaggedValue> second(thread, JSTaggedValue(groups));
    JSHandle<WeakVector> newVec1 = WeakVector::Append(thread,
        JSHandle<WeakVector>::Cast(info), first, ElementType::WEAK);
    JSHandle<WeakVector> newVec2 = WeakVector::Append(thread, newVec1, second);
    return JSHandle<DependentInfos>(newVec2);
}

// static
void DependentInfos::TraceLazyDeoptReason(JSThread *thread, JSHandle<JSFunction> func,
                                          DependentGroups groups)
{
    if (!thread->GetEcmaVM()->GetJSOptions().IsEnableLazyDeoptTrace()) {
        return;
    }
    JSTaggedValue funcName = JSFunction::NameGetter(thread, JSHandle<JSObject>::Cast(func));
    std::string funNameS = "Lazy Deoptimization occurred on";
    funNameS += " function: \"" + EcmaStringAccessor(funcName).ToStdString() + "\"";
    LOG_TRACE(INFO) << funNameS;
    // Get the lowbit of groups.
    uint32_t value = (static_cast<uint32_t>(groups) & (-static_cast<uint32_t>(groups)));
    std::string reason;
    switch (value) {
    #define LAZY_DEOPT_TYPE_CASE(name, value)           \
        case (value): {                                 \
            reason = #name;                             \
            break;                                      \
        }
        LAZY_DEOPT_TYPE_LIST(LAZY_DEOPT_TYPE_CASE)
    #undef LAZY_DEOPT_TYPE_CASE
        default:
            reason = "Unknown";
    }
    LOG_TRACE(INFO) << "Lazy Deoptimize reason: " << reason;
    std::string data = JsStackInfo::BuildJsStackTrace(thread, true);
    LOG_COMPILER(INFO) << "Lazy Deoptimize" << data;
}

// static
void DependentInfos::DeoptimizeGroups(JSHandle<DependentInfos> dependentInfos,
                                      JSThread *thread, DependentGroups groups)
{
    bool hasDeoptMethod = false;
    for (uint32_t i = 0; i < dependentInfos->GetEnd(); i += SLOT_PER_ENTRY) {
        DependentGroups depGroups = static_cast<DependentGroups>(dependentInfos->Get(i + GROUP_SLOT_OFFSET).GetInt());
        if (!CheckGroupsEffect(depGroups, groups)) {
            continue;
        }
        JSTaggedValue rawValue = dependentInfos->Get(i + FUNCTION_SLOT_OFFSET).GetWeakRawValue();
        if (!rawValue.IsHeapObject()) {
            continue;
        }
        JSHandle<JSFunction> func(thread, rawValue);
        if (func->IsCompiledCode()) {
            hasDeoptMethod = true;
            JSHandle<Method> method(thread, func->GetMethod());
            // When lazy deopt happened, the deopt method cannot call as jit any more.
            Deoptimizier::ClearCompiledCodeStatusWhenDeopt(thread,
                                                           func.GetObject<JSFunction>(),
                                                           method.GetObject<Method>(),
                                                           kungfu::DeoptType::LAZYDEOPT);
            TraceLazyDeoptReason(thread, func, (depGroups & groups));
        }
    }
    if (hasDeoptMethod) {
        // Replace return address
        Deoptimizier::PrepareForLazyDeopt(thread);
    }
}
}  // namespace panda::ecmascript