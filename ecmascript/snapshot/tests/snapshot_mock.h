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

#ifndef ECMASCRIPT_SNAPSHOT_SNAPSHOT_MOCK_H
#define ECMASCRIPT_SNAPSHOT_SNAPSHOT_MOCK_H

#include "ecmascript/ecma_vm.h"
#include "ecmascript/snapshot/mem/snapshot.h"

namespace panda::ecmascript {

class SnapshotMock : public Snapshot {
public:
    explicit SnapshotMock(EcmaVM *vm) : Snapshot(vm)
    {
        lastVersion_ = Snapshot::GetLastVersion();
    }

    std::string GetLastVersionStr()
    {
        return SnapShotHeader::ConvToStr(lastVersion_);
    }

    void SetLastVersion(base::FileHeaderBase::VersionType &lastVersion)
    {
        lastVersion_ = lastVersion;
    }

    const base::FileHeaderBase::VersionType &GetLastVersion() const override
    {
        return lastVersion_;
    }

private:
    base::FileHeaderBase::VersionType lastVersion_ {};
};

}  // namespace panda::ecmascript
#endif  // ECMASCRIPT_SNAPSHOT_SNAPSHOT_MOCK_H