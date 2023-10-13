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

#ifndef PROPERTY_DETECTOR_INL_H_
#define PROPERTY_DETECTOR_INL_H_

#include "ecmascript/property_detector.h"

#include "ecmascript/global_env.h"
#include "ecmascript/marker_cell.h"

namespace panda::ecmascript {
#define DETECTOR_DEFINITION(type, name, index)                                  \
    inline void PropertyDetector::Invalidate##name(JSHandle<GlobalEnv> env)     \
    {                                                                           \
        ASSERT(Is##name##Valid(env));                                           \
        JSTaggedValue value = env->GetTagged##name();                           \
        MarkerCell *cell = MarkerCell::Cast(value.GetTaggedObject());           \
        cell->InvalidatePropertyDetector();                                     \
    }                                                                           \
                                                                                \
    inline bool PropertyDetector::Is##name##Valid(JSHandle<GlobalEnv> env)      \
    {                                                                           \
        JSTaggedValue value = env->GetTagged##name();                           \
        MarkerCell *cell = MarkerCell::Cast(value.GetTaggedObject());           \
        return !cell->GetIsDetectorInvalid();                                   \
    }
GLOBAL_ENV_DETECTOR_FIELDS(DETECTOR_DEFINITION)
#undef DETECTOR_DEFINITION

} // namespace panda::ecmascript

#endif  // PROPERTY_DETECTOR_INL_H_
