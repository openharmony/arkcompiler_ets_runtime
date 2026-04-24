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

#ifndef ECMASCRIPT_ARKSTEED_BYTECODE_ITERATOR_H
#define ECMASCRIPT_ARKSTEED_BYTECODE_ITERATOR_H

#include "ecmascript/arksteed/arksteed_bytecode_context.h"
#include "ecmascript/compiler/bytecodes.h"
#include "ecmascript/mem/chunk_containers.h"

namespace panda::ecmascript::arksteed {
using namespace panda::ecmascript::kungfu;

class BytecodeIterator {
public:
    static constexpr uint32_t INVALID_INDEX = static_cast<uint32_t>(-1);

    BytecodeIterator() = default;
    explicit BytecodeIterator(BytecodeContext *bytecodeContext) : pos_(0), ctx_(bytecodeContext) {}

    BytecodeIterator &operator++()
    {
        pos_++;
        return *this;
    }

    BytecodeIterator &operator--()
    {
        pos_--;
        return *this;
    }

    void GotoStart()
    {
        pos_ = 0;
    }

    void GotoEnd()
    {
        auto &postOrderList = ctx_->GetPreprocessor().GetPostOrderList();
        pos_ = postOrderList.size() - 1;
    }

    void Goto(uint32_t index)
    {
        auto &index2PostOrderList = ctx_->GetPreprocessor().GetIndex2PostOrderList();
        pos_ = index2PostOrderList[index];
    }

    bool Done() const
    {
        auto &postOrderList = ctx_->GetPreprocessor().GetPostOrderList();
        return pos_ >= postOrderList.size();
    }

    uint32_t Index() const
    {
        auto &postOrderList = ctx_->GetPreprocessor().GetPostOrderList();
        return postOrderList[pos_];
    }

    uint32_t PrevIndex() const
    {
        auto &postOrderList = ctx_->GetPreprocessor().GetPostOrderList();
        ASSERT(postOrderList[pos_] > 0);
        return postOrderList[pos_] - 1;
    }

    uint32_t NextIndex() const
    {
        auto &postOrderList = ctx_->GetPreprocessor().GetPostOrderList();
        return postOrderList[pos_] + 1;
    }

    uint32_t NextRPOIndex() const
    {
        auto &postOrderList = ctx_->GetPreprocessor().GetPostOrderList();
        ASSERT(!Done());
        return postOrderList[pos_ + 1];
    }

    BytecodeInfo GetCurrentBytecodeInfo() const
    {
        auto &bytecodeInfo = ctx_->GetInfoData();
        uint32_t idx = Index();
        return bytecodeInfo[idx];
    }

    uint32_t GetJumpTargetBcIndex() const
    {
        return ctx_->GetJumpTargetBcIndex(Index());
    }

private:
    uint32_t pos_{0};
    BytecodeContext *ctx_{nullptr};
};

}  // namespace panda::ecmascript::arksteed

#endif  // ECMASCRIPT_ARKSTEED_BYTECODE_ITERATOR_H