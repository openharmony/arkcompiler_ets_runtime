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
#include "common_components/taskpool/taskpool.h"
#include "ecmascript/serializer/module_serializer.h"

namespace panda::ecmascript {
class ModuleSnapshot {
public:
    static constexpr std::string_view MODULE_SNAPSHOT_FILE_NAME = "Module.ams";
    static constexpr std::string_view SNAPSHOT_FILE_SUFFIX = ".ams"; // ark module snapshot

    static void SerializeDataAndPostSavingJob(const EcmaVM *vm, const CString &path, const CString &version);
    static void DeserializeData(const EcmaVM *vm, const CString &path, const CString &version);
    static JSHandle<TaggedArray> GetModuleSerializeArray(JSThread *thread);
    static void RemoveSnapshotFiles(const CString &path);

private:
    static constexpr int BUFFER_SIZE_INDEX = 0;
    static constexpr int BUFFER_CAPACITY_INDEX = 1;
    static constexpr int REGULAR_SPACE_SIZE_INDEX = 2;
    static constexpr int PIN_SPACE_SIZE_INDEX = 3;
    static constexpr int OLD_SPACE_SIZE_INDEX = 4;
    static constexpr int NONMOVABLE_SPACE_SIZE_INDEX = 5;
    static constexpr int MACHINECODE_SPACE_SIZE_INDEX = 6;
    static constexpr int SHARED_OLD_SPACE_SIZE_INDEX = 7;
    static constexpr int SHARED_NONMOVABLE_SPACE_SIZE_INDEX = 8;
    static constexpr int INCOMPLETE_DATA_INDEX = 9;
    static constexpr int GROUP_SIZE = 10;
    static constexpr int CMC_GC_REGION_SIZE = 2;
// Module snapshot layout
// +--------------------------------------+<-------- BaseInfo
// |       Application Version Code       |
// +--------------------------------------+
// |      System Version Code Length      |
// |         System Version Code          |
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
    static bool ReadDataFromFile(JSThread *thread, std::unique_ptr<SerializeData>& data, const CString &path,
        const CString &version);
    static bool WriteDataToFile(JSThread *thread, const std::unique_ptr<SerializeData>& data, const CString &filePath,
        const CString &version);

    class ModuleSnapshotTask : public common::Task {
    public:
        ModuleSnapshotTask(int32_t id, JSThread* thread, std::unique_ptr<SerializeData>& serializeData,
            const CString& path, const CString &version) : Task(id), thread_(thread),
            serializeData_(std::move(serializeData)), path_(path), version_(version) {}
        bool Run(uint32_t threadIndex) override;

        NO_COPY_SEMANTIC(ModuleSnapshotTask);
        NO_MOVE_SEMANTIC(ModuleSnapshotTask);

    private:
        JSThread* thread_ { nullptr };
        std::unique_ptr<SerializeData> serializeData_ {};
        CString path_ {};
        CString version_ {};
    };
};
}  // namespace panda::ecmascript
#endif // ECMASCRIPT_MODULE_MODULE_SNAPSHOT_H