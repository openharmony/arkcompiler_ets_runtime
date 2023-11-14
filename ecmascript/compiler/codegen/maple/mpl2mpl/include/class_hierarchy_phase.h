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

#ifndef MPL2MPL_INCLUDE_CLASS_HIERARCHY_PHASE_H
#define MPL2MPL_INCLUDE_CLASS_HIERARCHY_PHASE_H
#include "class_hierarchy.h"
#include "maple_phase.h"

namespace maple {
MAPLE_MODULE_PHASE_DECLARE_BEGIN(M2MKlassHierarchy)
KlassHierarchy *GetResult()
{
    return kh;
}
KlassHierarchy *kh = nullptr;
MAPLE_MODULE_PHASE_DECLARE_END
}  // namespace maple
#endif  // MPL2MPL_INCLUDE_CLASS_HIERARCHY_H
