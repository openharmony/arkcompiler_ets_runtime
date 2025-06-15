/*
 * Copyright (c) 2022 Huawei Device Co., Ltd.
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

#ifndef ECMASCRIPT_SNAPSHOT_MEM_SNAPSHOT_H
#define ECMASCRIPT_SNAPSHOT_MEM_SNAPSHOT_H

#include "ecmascript/compiler/aot_file/aot_version.h"
#include "ecmascript/common.h"
#include "ecmascript/mem/c_string.h"
#include "ecmascript/snapshot/mem/encode_bit.h"
#include "ecmascript/snapshot/mem/snapshot_processor.h"

#include "libpandabase/macros.h"

namespace panda::ecmascript {
class Program;
class EcmaVM;
class JSPandaFile;

class PUBLIC_API Snapshot {
public:
    explicit Snapshot(EcmaVM *vm) : vm_(vm) {}
    ~Snapshot() = default;

    void Serialize(const CString &fileName = "./snapshot");
    void Serialize(TaggedObject *objectHeader, const JSPandaFile *jsPandaFile, const CString &fileName = "./snapshot");
    void Serialize(uintptr_t startAddr, size_t size, const CString &fileName = "./snapshot");
    void SerializeBuiltins(const CString &fileName = "./snapshot");
    bool DeserializeInternal(SnapshotType type, const CString &snapshotFile, SnapshotProcessor &processor,
                             MemMap &fileMap);
    bool Deserialize(SnapshotType type, const CString &snapshotFile, bool isBuiltins = false);
#if defined(CROSS_PLATFORM) && defined(ANDROID_PLATFORM)
    bool Deserialize(SnapshotType type, const CString &snapshotFile, [[maybe_unused]] std::function<bool
        (std::string fileName, uint8_t **buff, size_t *buffSize)> ReadAOTCallBack, bool isBuiltins = false);
#endif
    JSTaggedValue GetDeserializeResultForUT() const
    {
        // ONLY used in UT to get the deserialize value result
        return result_;
    }

protected:
    struct SnapShotHeader : public base::FileHeaderBase {
    public:
        explicit SnapShotHeader(const VersionType &lastVersion) : base::FileHeaderBase(lastVersion) {}

        bool Verify(const VersionType &lastVersion) const
        {
            return VerifyVersion("snapshot file", lastVersion, AOTFileVersion::AI_STRICT_MATCH);
        }

        size_t regularObjSize {0};
        size_t pinnedObjSize {0};
        size_t largeObjSize {0};
        size_t oldSpaceObjSize {0};
        size_t nonMovableObjSize {0};
        size_t machineCodeObjSize {0};
        size_t snapshotObjSize {0};
        size_t hugeObjSize {0};
        size_t stringSize {0};
        uintptr_t pandaFileBegin {0};
        size_t rootObjectSize {0};
    };

    virtual const base::FileHeaderBase::VersionType &GetLastVersion() const
    {
        return AOTFileVersion::AI_VERSION;
    }

private:
    size_t AlignUpPageSize(size_t spaceSize);
    void WriteToFile(std::fstream &writer, const JSPandaFile *jsPandaFile, size_t size, SnapshotProcessor &processor);

    NO_MOVE_SEMANTIC(Snapshot);
    NO_COPY_SEMANTIC(Snapshot);

    EcmaVM *vm_;
    // ONLY used in UT to get the deserialize value result
    JSTaggedValue result_ {JSTaggedValue::Hole()};
};
}  // namespace panda::ecmascript
#endif  // ECMASCRIPT_SNAPSHOT_MEM_SNAPSHOT_H
