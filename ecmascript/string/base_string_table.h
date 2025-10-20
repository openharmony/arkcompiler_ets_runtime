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

#ifndef ECMASCRIPT_STRING_BASE_STRING_TABLE_H
#define ECMASCRIPT_STRING_BASE_STRING_TABLE_H

#include "objects/readonly_handle.h"
#include "thread/thread_holder.h"

namespace panda::ecmascript {
class BaseString;

template <typename Impl>
class BaseStringTableInterface {
public:
    BaseStringTableInterface() {}

    using HandleCreator = std::function<common::ReadOnlyHandle<BaseString>(common::ThreadHolder *, BaseString *)>;

    BaseString *GetOrInternFlattenString(common::ThreadHolder *holder, const HandleCreator &handleCreator,
        BaseString *string)
    {
        return static_cast<Impl *>(this)->GetOrInternFlattenString(holder, handleCreator, string);
    }

    BaseString *GetOrInternStringFromCompressedSubString(common::ThreadHolder *holder,
                                                         const HandleCreator &handleCreator,
                                                         const common::ReadOnlyHandle<BaseString> &string,
                                                         uint32_t offset, uint32_t utf8Len)
    {
        return static_cast<Impl *>(this)->GetOrInternStringFromCompressedSubString(
            holder, handleCreator, string, offset, utf8Len);
    }

    BaseString *GetOrInternString(common::ThreadHolder *holder, const HandleCreator &handleCreator,
                                  const uint8_t *utf8Data, uint32_t utf8Len, bool canBeCompress)
    {
        return static_cast<Impl *>(this)->GetOrInternString(holder, handleCreator, utf8Data, utf8Len, canBeCompress);
    }

    BaseString *GetOrInternString(common::ThreadHolder *holder, const HandleCreator &handleCreator,
                                  const uint16_t *utf16Data, uint32_t utf16Len, bool canBeCompress)
    {
        return static_cast<Impl *>(this)->GetOrInternString(holder, handleCreator, utf16Data, utf16Len, canBeCompress);
    }

    BaseString *TryGetInternString(const common::ReadOnlyHandle<BaseString> &string)
    {
        return static_cast<Impl *>(this)->TryGetInternString(string);
    }

    // Runtime module interfaces
    void Init() {};

    void Fini() {};

private:
    NO_COPY_SEMANTIC_CC(BaseStringTableInterface);
    NO_MOVE_SEMANTIC_CC(BaseStringTableInterface);
};
}
#endif //ECMASCRIPT_STRING_BASE_STRING_TABLE_H