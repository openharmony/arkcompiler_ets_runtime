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

#ifndef ECMASCRIPT_ARKSTEED_PROCESSED_FEEDBACK_H
#define ECMASCRIPT_ARKSTEED_PROCESSED_FEEDBACK_H

#include <array>

#include "ecmascript/arksteed/arksteed_pgo_access_info.h"
#include "ecmascript/js_tagged_value.h"

namespace panda::ecmascript::arksteed {

enum class ProcessedFeedbackKind : uint8_t {
    INSUFFICIENT,
    NAMED_ACCESS,
};

struct ProcessedFeedbackBase {
    ProcessedFeedbackKind kind {ProcessedFeedbackKind::INSUFFICIENT};
    AccessFeedbackSource source {};

    bool IsInsufficient() const
    {
        return kind == ProcessedFeedbackKind::INSUFFICIENT;
    }
};

struct NamedAccessCaseFeedback {
    ArkSteedHClassRef expectedHClass {};
    ArkSteedHandlerRef handler {};
};

struct NamedAccessFeedback {
    ProcessedFeedbackBase base {};
    ArkSteedNameRef name {};
    std::array<NamedAccessCaseFeedback, MAX_NAMED_IC_POLY_CASES> cases {};
    uint32_t caseCount {0};
};

}  // namespace panda::ecmascript::arksteed

#endif  // ECMASCRIPT_ARKSTEED_PROCESSED_FEEDBACK_H
