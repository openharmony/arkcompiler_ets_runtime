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

#ifndef ECMASCRIPT_ARKSTEED_FEEDBACK_READER_H
#define ECMASCRIPT_ARKSTEED_FEEDBACK_READER_H

#include <variant>

#include "ecmascript/arksteed/arksteed_heap_broker.h"
#include "ecmascript/arksteed/arksteed_processed_feedback.h"
#include "ecmascript/compiler/bytecodes.h"
#include "ecmascript/compiler/jit_compilation_env.h"
#include "ecmascript/ic/ic_handler.h"
#include "ecmascript/ic/profile_type_info.h"
#include "ecmascript/js_thread.h"
#include "ecmascript/tagged_array.h"

namespace panda::ecmascript::arksteed {

class ArkSteedFeedbackReader {
public:
    ArkSteedFeedbackReader(JSThread *compilerThread, const panda::ecmascript::kungfu::BytecodeInfo &bytecodeInfo,
                           ArkSteedHeapBroker *broker)
        : compilerThread_(compilerThread), bytecodeInfo_(bytecodeInfo), broker_(broker)
    {}

    bool ReadNamedAccessFeedback(int slotIndex, NamedAccessFeedback *feedback) const;
    bool TryGetFeedbackSlotId(int index, bool allowImmediate, uint32_t *slotId) const;

private:
    struct NamedICMonoSnapshot {
        ArkSteedHClassRef expectedHClass;
        JSTaggedValue handler;
        uint32_t slotId {0};
    };

    struct NamedICCaseSnapshot {
        ArkSteedHClassRef expectedHClass;
        JSTaggedValue handler;
    };

    struct NamedICPolySnapshot {
        TaggedArray *polyArray {nullptr};
        uint32_t caseCount {0};
        uint32_t slotId {0};
    };

    bool TryReadNamedAccessName(ArkSteedNameRef *name) const;
    bool TryGetConstDataId(int index, uint16_t *constDataId) const;
    NamedAccessCaseFeedback MakeNamedAccessCaseFeedback(ArkSteedHClassRef expectedHClass,
                                                        JSTaggedValue handler) const;
    bool TryGetNamedICMonoSnapshot(int slotIndex, NamedICMonoSnapshot *snapshot) const;
    bool TryGetNamedICPolySnapshot(int slotIndex, NamedICPolySnapshot *snapshot) const;
    bool TryGetNamedICPolyCase(const NamedICPolySnapshot &snapshot, uint32_t caseIndex,
                               NamedICCaseSnapshot *icCase) const;

    JSThread *compilerThread_ {nullptr};
    const panda::ecmascript::kungfu::BytecodeInfo &bytecodeInfo_;
    ArkSteedHeapBroker *broker_ {nullptr};
};

}  // namespace panda::ecmascript::arksteed

#endif  // ECMASCRIPT_ARKSTEED_FEEDBACK_READER_H
