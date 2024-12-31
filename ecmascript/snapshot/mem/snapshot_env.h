/*
 * Copyright (c) 2021 Huawei Device Co., Ltd.
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

#ifndef ECMASCRIPT_SNAPSHOT_MEM_SNAPSHOT_ENV_H
#define ECMASCRIPT_SNAPSHOT_MEM_SNAPSHOT_ENV_H

#include <unordered_map>

#include "ecmascript/js_thread.h"
#include "ecmascript/mem/visitor.h"

namespace panda::ecmascript {
class EcmaVM;

class SnapshotEnv final {
public:
    explicit SnapshotEnv(EcmaVM *vm) : vm_(vm) {}
    ~SnapshotEnv() = default;

    void AddGlobalConstToMap();

    void ClearEnvMap()
    {
        rootObjectMap_.clear();
    }

    void Iterate(RootVisitor &v, VMRootVisitType type);

    void Push(JSTaggedType objectAddr, uint32_t index)
    {
        if (multiThreadCheckValue_.exchange(JSThread::GetCurrentThreadId()) != 0) {
            LOG_ECMA(FATAL) << "SnapshotEnv push multi-thread check fail, thread id: " << multiThreadCheckValue_;
        }
        rootObjectMap_.emplace(objectAddr, index);
        multiThreadCheckValue_ = 0;
    }

    void Remove(JSTaggedType objectAddr)
    {
        if (multiThreadCheckValue_.exchange(JSThread::GetCurrentThreadId()) != 0) {
            LOG_ECMA(FATAL) << "SnapshotEnv remove multi-thread check fail, thread id: " << multiThreadCheckValue_;
        }
        rootObjectMap_.erase(objectAddr);
        multiThreadCheckValue_ = 0;
    }

    uint32_t FindEnvObjectIndex(JSTaggedType objectAddr) const
    {
        if (rootObjectMap_.find(objectAddr) != rootObjectMap_.end()) {
            return rootObjectMap_.find(objectAddr)->second;
        }
        return MAX_UINT_32;
    }

    JSTaggedType RelocateRootObjectAddr(uint32_t index);

    static constexpr size_t MAX_UINT_32 = 0xFFFFFFFF;

private:
    NO_MOVE_SEMANTIC(SnapshotEnv);
    NO_COPY_SEMANTIC(SnapshotEnv);

    void InitGlobalConst();
    void InitGlobalEnv();

    EcmaVM *vm_;
    std::unordered_map<JSTaggedType, uint32_t> rootObjectMap_;
    std::atomic<uint32_t> multiThreadCheckValue_ {0};
};
}  // namespace panda::ecmascript
#endif  // ECMASCRIPT_SNAPSHOT_MEM_SNAPSHOT_ENV_H
