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
#ifndef ECMASCRIPT_PANDAFILE_JS_PANDAFILE_SNAPSHOT_H
#define ECMASCRIPT_PANDAFILE_JS_PANDAFILE_SNAPSHOT_H

#include "common_components/taskpool/task.h"
#include "ecmascript/ecma_vm.h"
#include "ecmascript/jspandafile/js_pandafile.h"

namespace panda::ecmascript {
class JSPandaFileSnapshot {
public:
    static constexpr std::string_view JSPANDAFILE_FILE_NAME = "_Pandafile.ams";
    static constexpr std::string_view SNAPSHOT_FILE_SUFFIX = ".ams"; // ark module snapshot

    static void PostWriteDataToFileJob(const EcmaVM *vm, const CString &path);
    static bool ReadData(JSThread *thread, JSPandaFile *jsPandaFile, const CString &path);

private:
    static constexpr uint32_t VERSION_CODE_COUNT = 2; // application versionCode + persist versionCode
    static constexpr size_t VERSION_SIZE = 4;
    using Version = std::array<uint8_t, VERSION_SIZE>;
    static constexpr Version JSPANDAFILE_SNAPSHOT_VERSION_CODE = { 0, 0, 0, 1 };
    inline static uint32_t GetVersionCode()
    {
        return bit_cast<uint32_t>(JSPANDAFILE_SNAPSHOT_VERSION_CODE);
    }
    static bool IsJSPandaFileSnapshotFileExist(const CString &fileName, const CString &path);
    static CString GetJSPandaFileFileName(const CString &fileName, const CString &path);
    static void RemoveSnapshotFiles(const CString &path);

// JSPandaFile snapshot layout
// +---------------------------------+<-------- BaseInfo
// |    Application Version Code     |
// +---------------------------------+
// |      Snapshot Version Code      |
// +---------------------------------+
// |        JSPandaFile Size         |
// +---------------------------------+
// |           moduleName            |
// +---------------------------------+<-------- MethodLiterals
// |           numMethods            |
// +---------------------------------+
// |          MethodLiteral          |
// |               ...               |
// +---------------------------------+<-------- MainMethodIndex
// |      MainMethodIndex size       |
// +---------------------------------+
// |         MainMethodIndex         |
// |        RecordName Lenth         |
// |         RecordName ptr          |
// |               ...               |
// +---------------------------------+<-------- CheckSum
// |             CheckSum            |
// +---------------------------------+
    static void WriteDataToFile(JSThread *thread, JSPandaFile *jsPandaFile, const CString &path);
    static bool ReadDataFromFile(JSThread *thread, JSPandaFile *jsPandaFile, const CString &path);

    class JSPandaFileSnapshotTask : public common::Task {
    public:
        JSPandaFileSnapshotTask(int32_t id, JSThread *thread, JSPandaFile *jsPandaFile, const CString &path)
            : Task(id), thread_(thread), jsPandaFile_(jsPandaFile), path_(path) {}
        ~JSPandaFileSnapshotTask() override = default;
        bool Run(uint32_t threadIndex) override;

        NO_COPY_SEMANTIC(JSPandaFileSnapshotTask);
        NO_MOVE_SEMANTIC(JSPandaFileSnapshotTask);

    private:
        JSThread *thread_ {nullptr};
        JSPandaFile *jsPandaFile_ {nullptr};
        CString path_ {};
    };
};
}  // namespace panda::ecmascript
#endif // ECMASCRIPT_PANDAFILE_JS_PANDAFILE_SNAPSHOT_H