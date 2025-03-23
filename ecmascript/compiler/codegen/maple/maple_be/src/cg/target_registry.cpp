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
#include "target_registry.h"

namespace maplebe {
using namespace maple;
static Target *firstTarget = nullptr;
void TargetRegistry::RegisterTarget(Target &t, const std::string name)
{
    // Add to the list of targets.
    t.next = firstTarget;
    firstTarget = &t;
    t.name = name;
}

// todo: this is an ugly implementation, when there is time, fix this to iterator and findif
Target *TargetRegistry::lookupTarget(const std::string &targetName)
{
    Target* cur = firstTarget;
    while (cur != nullptr) {
        if (cur->getName() == targetName) {
            return cur;
        }
        cur = cur->next;
    }
    CHECK_FATAL_FALSE("I did not find the target! maybe you did not build the target?");
}


}  /* namespace maplebe */
