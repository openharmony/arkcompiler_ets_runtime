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

#include "ecmascript/compiler/debug_info.h"

namespace panda::ecmascript::kungfu {
std::string DebugInfo::FuncDebugInfo::EMPTY_COMMENT = "";

DebugInfo::DebugInfo(NativeAreaAllocator* allocator, bool enable)
    : chunk_(allocator),
      funcToDInfo_(&chunk_),
      dInfos_(&chunk_),
      enable_(enable)
{
}

DebugInfo::~DebugInfo()
{
    for (auto info : dInfos_) {
        if (info != nullptr) {
            delete info;
        }
    }
    dInfos_.clear();
}

void DebugInfo::AddFuncName(const std::string &name)
{
    ASSERT(enable_);
    if (dInfos_.size() > 0) {
        FuncDebugInfo *info = dInfos_.back();
        info->SetName(name);
        ASSERT(funcToDInfo_.find(name) == funcToDInfo_.end());
        size_t index = dInfos_.size() - 1;
        funcToDInfo_[name] = index;
    }
}

size_t DebugInfo::AddComment(std::string &&str)
{
    ASSERT(enable_);
    ASSERT(dInfos_.size() > 0);
    FuncDebugInfo *info = dInfos_.back();
    size_t index = info->Add(std::move(str));
    return index;
}

void DebugInfo::AppendComment(size_t index, std::string &&str, std::string_view separator)
{
    ASSERT(enable_);
    ASSERT(dInfos_.size() > 0);
    FuncDebugInfo *info = dInfos_.back();
    info->Append(index, std::move(str), separator);
}

void DebugInfo::AddFuncDebugInfo(const std::string& name)
{
    ASSERT(enable_);
    FuncDebugInfo *info = new FuncDebugInfo(&chunk_);
    dInfos_.push_back(info);
    AddFuncName(name);
}

const std::string &DebugInfo::GetComment(const std::string &funcName, size_t index) const
{
    auto it = funcToDInfo_.find(funcName);
    if (it != funcToDInfo_.end()) {
        ASSERT(dInfos_.size() > it->second);
        FuncDebugInfo* info = dInfos_[it->second];
        ASSERT(info->Name() == funcName);
        return info->GetComment(index);
    }
    return FuncDebugInfo::EmptyComment();
}

const std::string &DebugInfo::GetComment(size_t index) const
{
    ASSERT(!dInfos_.empty());
    auto *info = dInfos_.back();
    return info->GetComment(index);
}
}  // namespace panda::ecmascript::kungfu
