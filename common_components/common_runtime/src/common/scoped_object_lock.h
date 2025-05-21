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
#ifndef ARK_COMMON_SCOPED_LOCK_OBJECT_H
#define ARK_COMMON_SCOPED_LOCK_OBJECT_H
#include "common_components/common_runtime/src/common/base_object.h"
#include "common_components/log/log.h"
namespace panda {
class ScopedObjectLock {
public:
    NO_INLINE_CC explicit ScopedObjectLock(BaseObject& obj)
    {
        do {
            panda::BaseStateWord::ForwardState state = obj.GetBaseStateWord().GetForwardState();
            if (state == panda::BaseStateWord::ForwardState::FORWARDING ||
                state == panda::BaseStateWord::ForwardState::FORWARDED ||
                state == panda::BaseStateWord::ForwardState::NORMAL) {
                lockedObj_ = &obj;
            } else {
                LOG_COMMON(FATAL) << "this state need to be dealt with when lock object, state: " <<
                    static_cast<uint8_t>(state);
                return;
            }
            panda::BaseStateWord curState = lockedObj_->GetBaseStateWord();
            if (lockedObj_->TryLockExclusive(curState)) {
                return;
            }
        } while (true);
    }
    ~ScopedObjectLock()
    {
        LOGF_CHECK(lockedObj_ != nullptr) << "from copy is nullptr when unlock object\n";
        lockedObj_->UnlockExclusive(panda::BaseStateWord::ForwardState::NORMAL);
    }

private:
    BaseObject* lockedObj_ = { nullptr };
};
} // namespace panda
#endif // ~ARK_COMMON_SCOPED_LOCK_OBJECT_H
