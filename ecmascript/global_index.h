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

#ifndef ECMASCRIPT_GLOBAL_INDEX_H
#define ECMASCRIPT_GLOBAL_INDEX_H

namespace panda::ecmascript {
class GlobalIndex {
public:
    static constexpr uint32_t GLOBAL_CONST_BITFILED_NUM = 10;
    static constexpr uint32_t GLOBAL_ENV_BITFILED_NUM = 10;
    static constexpr uint32_t BUILTIN_ENTRIES_BITFILED_NUM = 10;
    using GlobalConstBits = BitField<size_t, 0, GLOBAL_CONST_BITFILED_NUM>;
    using GlobalEnvBits = GlobalConstBits::NextField<size_t, GLOBAL_ENV_BITFILED_NUM>;
    using BuiltinEntriesBits = GlobalEnvBits::NextField<size_t, BUILTIN_ENTRIES_BITFILED_NUM>;

    GlobalIndex() = default;
    explicit GlobalIndex(uint32_t index) : index_(index) {}
    explicit GlobalIndex(int32_t index) : index_(static_cast<uint32_t>(index)) {}

    int GetGlobalConstId() const
    {
        int id = static_cast<int>(GlobalConstBits::Decode(index_));
        return id - 1;
    }

    void UpdateGlobalConstId(size_t id)
    {
        index_ = GlobalConstBits::Update(index_, id + 1);
    }

    bool IsGlobalConstId() const
    {
        return GlobalConstBits::Decode(index_);
    }

    int GetGlobalEnvId() const
    {
        int id = static_cast<int>(GlobalEnvBits::Decode(index_));
        return id - 1;
    }

    void UpdateGlobalEnvId(size_t id)
    {
        index_ = GlobalEnvBits::Update(index_, id + 1);
    }

    bool IsGlobalEnvId() const
    {
        return GlobalEnvBits::Decode(index_);
    }

    int GetBuiltinEntriesId() const
    {
        int id = static_cast<int>(BuiltinEntriesBits::Decode(index_));
        return id - 1;
    }

    void UpdateBuiltinEntriesId(size_t id)
    {
        index_ = BuiltinEntriesBits::Update(index_, id + 1);
    }

    bool IsBuiltinEntriesId() const
    {
        return BuiltinEntriesBits::Decode(index_);
    }

    uint32_t GetGlobalIndex() const
    {
        return index_;
    }

    void UpdateGlobalIndex(size_t index)
    {
        index_ = index;
    }

    bool operator!=(const GlobalIndex &right) const
    {
        return index_ != right.index_;
    }

    bool operator==(const GlobalIndex &right) const
    {
        return index_ == right.index_;
    }

private:
    uint32_t index_ {0};
};
}  // namespace panda::ecmascript

#endif  // ECMASCRIPT_GLOBAL_INDEX_MAP_H