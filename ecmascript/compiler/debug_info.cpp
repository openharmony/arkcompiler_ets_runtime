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
DebugInfo::DebugInfo(NativeAreaAllocator* allocator)
    : chunk_(allocator),
      funcToDInfo_(&chunk_),
      dInfos_(&chunk_)
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
    ASSERT(dInfos_.size() > 1);
    if (dInfos_.size() > 1) {
        FuncDebugInfo *info = dInfos_.back();
        ASSERT(info->Name().empty());
        info->SetName(name);
        ASSERT(funcToDInfo_.find(name) == funcToDInfo_.end());
        size_t index = dInfos_.size() - 1;
        funcToDInfo_[name] = index;
    }
}

size_t DebugInfo::AddComment(const char* str)
{
    ASSERT(dInfos_.size() > 1);
    FuncDebugInfo *info = dInfos_.back();
    ASSERT(info->Name().empty());
    size_t index = info->Add(str);
    return index;
}

void DebugInfo::AddFuncDebugInfo()
{
    FuncDebugInfo *info = new FuncDebugInfo(&chunk_);
    dInfos_.push_back(info);
}
}  // namespace panda::ecmascript::kungfu
