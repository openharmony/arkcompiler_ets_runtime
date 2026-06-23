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

#ifndef ECMASCRIPT_ARKSTEED_BC_FRAME_STATE_H
#define ECMASCRIPT_ARKSTEED_BC_FRAME_STATE_H

#include "ecmascript/arksteed/arksteed_vreg.h"
#include "ecmascript/compiler/base/bit_set.h"

namespace panda::ecmascript::arksteed {

class ValueVertex;

class SharedBCFrameState {
public:
    SharedBCFrameState(VRegIDType numVRegs, ValueVertex *initial, Chunk *chunk)
    {
        data_ = chunk->NewArray<ValueVertex *>(numVRegs);
        numVRegs_ = numVRegs;
        std::fill_n(data_, numVRegs_, initial);
    }

    DEFAULT_COPY_SEMANTIC(SharedBCFrameState);
    DEFAULT_MOVE_SEMANTIC(SharedBCFrameState);

    void Reset(ValueVertex *initial)
    {
        std::fill_n(data_, numVRegs_, initial);
    }

    VRegIDType NumVRegs() const
    {
        return numVRegs_;
    }

    ValueVertex *Get(VRegIDType index) const
    {
        ASSERT(index < numVRegs_);
        return data_[index];
    }
    ValueVertex *GetLexicalEnv() const
    {
        return data_[numVRegs_ - EXTRA_VREG_COUNT + LEXICAL_ENV_EXTRA_INDEX];
    }
    ValueVertex *GetAcc() const
    {
        return data_[numVRegs_ - EXTRA_VREG_COUNT + ACC_EXTRA_INDEX];
    }

    void Set(VRegIDType index, ValueVertex *vertex)
    {
        ASSERT(index < numVRegs_);
        data_[index] = vertex;
    }
    void SetLexicalEnv(ValueVertex *vertex)
    {
        data_[numVRegs_ - EXTRA_VREG_COUNT + LEXICAL_ENV_EXTRA_INDEX] = vertex;
    }
    void SetAcc(ValueVertex *vertex)
    {
        data_[numVRegs_ - EXTRA_VREG_COUNT + ACC_EXTRA_INDEX] = vertex;
    }

private:
    // Layout: [Locals] [Params] [LexicalEnv] [Acc]
    ValueVertex **data_ = nullptr;
    VRegIDType numVRegs_ = 0;
};

class CondensedBCFrameState {
public:
    // IsInitialized() == false
    CondensedBCFrameState() = default;

    // IsInitialized() == true
    CondensedBCFrameState(SharedBCFrameState from, const kungfu::BitSet &liveSet, Chunk *chunk)
    {
        numLiveVRegs_ = liveSet.Count();
        data_ = chunk->NewArray<CondensedEntry>(numLiveVRegs_);

        VRegIDType numVRegs = from.NumVRegs();
        VRegIDType liveCount = 0;
        for (VRegIDType vregIndex = 0; vregIndex < numVRegs; vregIndex++) {
            if (liveSet.TestBit(vregIndex)) {
                data_[liveCount++] = {.vertex = from.Get(vregIndex), .index = vregIndex};
            }
        }
        ASSERT(liveCount == numLiveVRegs_);
    }

    DEFAULT_COPY_SEMANTIC(CondensedBCFrameState);  // Shallow copy
    DEFAULT_MOVE_SEMANTIC(CondensedBCFrameState);

    template <typename Function>
    void ForEach(Function &&f) const
    {
        static_assert(std::is_invocable_v<Function, ValueVertex *, VRegIDType>);
        for (VRegIDType i = 0; i < numLiveVRegs_; i++) {
            std::invoke(f, data_[i].vertex, data_[i].index);
        }
    }

    VRegIDType NumLiveVRegs() const
    {
        return numLiveVRegs_;
    }

    bool IsInitialized() const
    {
        return data_ != nullptr;
    }

private:
    struct CondensedEntry {
        ValueVertex *vertex;
        VRegIDType index;
    };

    CondensedEntry *data_ = nullptr;
    VRegIDType numLiveVRegs_ = 0;
};

}  // namespace panda::ecmascript::arksteed

#endif  // ECMASCRIPT_ARKSTEED_BC_FRAME_STATE_H
