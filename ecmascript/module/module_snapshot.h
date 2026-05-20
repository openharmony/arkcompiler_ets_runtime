/*
 * Copyright (c) 2025 Huawei Device Co., Ltd.
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
#ifndef ECMASCRIPT_MODULE_MODULE_SNAPSHOT_H
#define ECMASCRIPT_MODULE_MODULE_SNAPSHOT_H

#include "ecmascript/ecma_vm.h"

namespace panda::ecmascript {
class SerializeData;

class ModuleSnapshot {
public:
    static constexpr std::string_view MODULE_SNAPSHOT_FILE_NAME = "Module.ams";
    static constexpr std::string_view SNAPSHOT_FILE_SUFFIX = ".ams"; // ark module snapshot

    static void SerializeDataAndPostSavingJob(const EcmaVM *vm, const CString &path, const CString &version);
    static bool DeserializeData(const EcmaVM *vm, const CString &path, const CString &version);

protected:
// Module snapshot layout
// +--------------------------------------+<-------- BaseInfo
// |        Snapshot Version Info         | @see: ModulesSnapshotHelper::SnapshotVersionInfo
// +--------------------------------------+<-------- data
// |               dataIndex              |
// +--------------------------------------+
// |               sizeLimit              |
// +--------------------------------------+
// |              bufferSize              |
// |            bufferCapacity            |
// |           regularSpaceSize           |
// |             pinSpaceSize             |
// |             oldSpaceSize             |
// |          nonMovableSpaceSize         |
// |          machineCodeSpaceSize        |
// |           sharedOldSpaceSize         |
// |       sharedNonMovableSpaceSize      |
// |             incompleteData           |
// +--------------------------------------+<-------- CMCGC
// |     regularRemainSizeVector size     |
// |       regularRemainSizeVector        |
// |     regularRemainSizeVector size     |
// |         pinRemainSizeVector          |
// +--------------------------------------+<-------- GC
// |     regionRemainSizeVectors size     |
// |       regionRemainSizeVectors        |
// +--------------------------------------+<-------- data
// |                buffer                |
// +--------------------------------------+<-------- CheckSum
// |               CheckSum               |
// +--------------------------------------+
    static JSHandle<TaggedArray> GetModuleSerializeArray(JSThread *thread);
    static void RestoreUpdatedBinding(JSThread* thread, JSHandle<TaggedArray> serializeArray);
    static std::unique_ptr<SerializeData> GetSerializeData(JSThread *thread);
};
}  // namespace panda::ecmascript
#endif // ECMASCRIPT_MODULE_MODULE_SNAPSHOT_H