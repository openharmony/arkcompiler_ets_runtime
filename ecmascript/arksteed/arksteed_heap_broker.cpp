/*
 * Copyright (c) 2026 Huawei Device Co., Ltd.
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

#include "ecmascript/arksteed/arksteed_heap_broker.h"

#include "ecmascript/arksteed/arksteed_feedback_reader.h"

namespace panda::ecmascript::arksteed {

bool ArkSteedHeapBroker::GetFeedbackForNamedAccess(const ArkSteedFeedbackReader &reader, int slotIndex,
                                                   NamedAccessFeedback *feedback) const
{
    *feedback = {};
    uint32_t slotId = 0;
    if (!reader.TryGetFeedbackSlotId(slotIndex, false, &slotId)) {
        return false;
    }
    if (TryGetCachedNamedAccessFeedback({slotId, false}, feedback) ||
        TryGetCachedNamedAccessFeedback({slotId, true}, feedback)) {
        return true;
    }

    SerializingScope scope(this, "ArkSteedHeapBroker::GetFeedbackForNamedAccess");
    bool success = reader.ReadNamedAccessFeedback(slotIndex, feedback);
    if (!success) {
        *feedback = {};
        return false;
    }
    CacheNamedAccessFeedback(*feedback);
    return true;
}

}  // namespace panda::ecmascript::arksteed
