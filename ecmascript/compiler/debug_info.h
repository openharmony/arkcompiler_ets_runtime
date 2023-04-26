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

#ifndef ECMASCRIPT_COMPILER_DEBUG_INFO_H
#define ECMASCRIPT_COMPILER_DEBUG_INFO_H

#include "ecmascript/mem/chunk_containers.h"

namespace panda::ecmascript::kungfu {
class DebugInfo {
public:
    DebugInfo(NativeAreaAllocator* allocator, bool enable = false);
    ~DebugInfo();
    bool IsEnable() const
    {
        return enable_;
    }
    size_t AddComment(const char* str);
    void AddFuncDebugInfo(const std::string &name);
    const std::string &GetComment(const std::string &funcName, size_t index) const;

private:
    class FuncDebugInfo {
    public:
        FuncDebugInfo(Chunk *chunk) : chunk_(chunk), name_("")
        {
            comments_ = new ChunkVector<std::string>(chunk_);
        }

        ~FuncDebugInfo()
        {
            if (comments_ != nullptr) {
                delete comments_;
                comments_ = nullptr;
            }
        }

        const std::string &Name() const
        {
            return name_;
        }

        void SetName(const std::string &n)
        {
            name_ = n;
        }

        size_t Add(const std::string &str)
        {
            comments_->push_back(str);
            return comments_->size() - 1;
        }

        const std::string &GetComment(size_t index)
        {
            ASSERT(comments_ != nullptr);
            if (index < comments_->size()) {
                return comments_->at(index);
            }
            return EmptyComment();
        }

        static const std::string &EmptyComment()
        {
            return EMPTY_COMMENT;
        }

    private:
        static std::string EMPTY_COMMENT;
        Chunk* chunk_ {nullptr};
        std::string name_;
        ChunkVector<std::string> *comments_ {nullptr};
    };

    void AddFuncName(const std::string &name);

    Chunk chunk_;
    ChunkMap<std::string, size_t> funcToDInfo_;
    ChunkVector<FuncDebugInfo*> dInfos_;
    bool enable_ {false};
};
}  // namespace panda::ecmascript::kungfu
#endif  // ECMASCRIPT_COMPILER_DEBUG_INFO_H

