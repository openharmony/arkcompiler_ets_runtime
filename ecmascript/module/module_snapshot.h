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

    static void SerializeDataAndPostSavingJob(const EcmaVM *vm, const CString &path);
    static void DeserializeData(const EcmaVM *vm, const CString &path);
    static JSHandle<TaggedArray> GetModuleSerializeArray(JSThread *thread);
    static void RemoveSnapshotFiles(const CString &path);

private:
    static constexpr size_t VERSION_SIZE = 4;
    using Version = std::array<uint8_t, VERSION_SIZE>;
    static constexpr Version MODULE_SNAPSHOT_VERSION_CODE = { 0, 0, 0, 1 };
    inline static uint32_t GetVersionCode()
    {
        return bit_cast<uint32_t>(MODULE_SNAPSHOT_VERSION_CODE);
    }
    static constexpr int BUFFER_SIZE_INDEX = 0;
    static constexpr int BUFFER_CAPACITY_INDEX = 1;
    static constexpr int OLD_SPACE_SIZE_INDEX = 2;
    static constexpr int NONMOVABLE_SPACE_SIZE_INDEX = 3;
    static constexpr int MACHINECODE_SPACE_SIZE_INDEX = 4;
    static constexpr int SHARED_OLD_SPACE_SIZE_INDEX = 5;
    static constexpr int SHARED_NONMOVABLE_SPACE_SIZE_INDEX = 6;
    static constexpr int INCOMPLETE_DATA_INDEX = 7;

    static bool ReadDataFromFile(JSThread *thread, std::unique_ptr<SerializeData>& data, const CString &path);
    static bool WriteDataToFile(JSThread *thread, const std::unique_ptr<SerializeData>& data, const CString &filePath);

    class ModuleSnapshotTask : public common::Task {
    public:
        ModuleSnapshotTask(int32_t id, JSThread* thread, std::unique_ptr<SerializeData>& serializeData,
            const CString& path) : Task(id), thread_(thread), serializeData_(std::move(serializeData)), path_(path) {}
        bool Run(uint32_t threadIndex) override;

        NO_COPY_SEMANTIC(ModuleSnapshotTask);
        NO_MOVE_SEMANTIC(ModuleSnapshotTask);

    private:
        JSThread* thread_ { nullptr };
        std::unique_ptr<SerializeData> serializeData_ {};
        CString path_ {};
    };
};
}  // namespace panda::ecmascript
#endif // ECMASCRIPT_MODULE_MODULE_SNAPSHOT_H