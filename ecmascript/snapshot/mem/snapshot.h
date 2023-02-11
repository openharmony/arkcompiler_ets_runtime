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

#include "ecmascript/common.h"
#include "ecmascript/snapshot/mem/encode_bit.h"
#include "ecmascript/snapshot/mem/snapshot_processor.h"
#include "ecmascript/mem/c_string.h"

#include "libpandabase/macros.h"

namespace panda::ecmascript {
class Program;
class EcmaVM;
class JSPandaFile;

class PUBLIC_API Snapshot final {
public:
    explicit Snapshot(EcmaVM *vm) : vm_(vm) {}
    ~Snapshot() = default;

    void Serialize(const CString &fileName = "./snapshot");
    void Serialize(TaggedObject *objectHeader, const JSPandaFile *jsPandaFile, const CString &fileName = "./snapshot");
    void Serialize(uintptr_t startAddr, size_t size, const CString &fileName = "./snapshot");
    void SerializeBuiltins(const CString &fileName = "./snapshot");
    const JSPandaFile *Deserialize(SnapshotType type, const CString &snapshotFile, bool isBuiltins = false);

private:
    struct SnapShotHeader : public base::FileHeader {
    public:
        static constexpr std::array<uint8_t, VERSION_SIZE> LAST_VERSION = {0, 0, 0, 1};

        SnapShotHeader() : base::FileHeader(LAST_VERSION) {}

        bool Verify()
        {
            return VerifyInner(LAST_VERSION);
        }

        uint32_t oldSpaceObjSize;
        uint32_t nonMovableObjSize;
        uint32_t machineCodeObjSize;
        uint32_t snapshotObjSize;
        uint32_t hugeObjSize;
        uint32_t stringSize;
        uint32_t pandaFileBegin;
        uint32_t rootObjectSize;
    };

private:
    size_t AlignUpPageSize(size_t spaceSize);
    void WriteToFile(std::fstream &writer, const JSPandaFile *jsPandaFile, size_t size, SnapshotProcessor &processor);

    NO_MOVE_SEMANTIC(Snapshot);
    NO_COPY_SEMANTIC(Snapshot);

    EcmaVM *vm_;
};
}  // namespace panda::ecmascript
#endif  // ECMASCRIPT_SNAPSHOT_MEM_SNAPSHOT_H
