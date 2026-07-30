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
#include "ecmascript/platform/file.h"
#include "ecmascript/snapshot/common/modules_snapshot_helper.h"

namespace panda::ecmascript {
class JSPandaFileSnapshot {
public:
    static constexpr std::string_view JSPANDAFILE_FILE_NAME = "_Pandafile.ams";
    static constexpr std::string_view CROSS_BUNDLE_HSP_FILE_NAME = "CrossBundleHsp.ams";

    static void PostWriteDataToFileJob(const EcmaVM *vm, const CString &path, const CString &version);
    static bool ReadData(JSThread *thread, JSPandaFile *jsPandaFile, const CString &path, const CString &version);
    static bool ReadCrossBundleHspDataFromFile(JSThread *thread, const CString &path, const CString &version);
protected:
    static bool IsJSPandaFileSnapshotFileExist(const CString &fileName, const CString &path);
    static CString GetJSPandaFileFileName(const CString &fileName, const CString &path);

// JSPandaFile snapshot layout
// +---------------------------------+<-------- BaseInfo
// |    Application Version Code     |
// +---------------------------------+
// |   System Version Code Length    |
// |      System Version Code        |
// +---------------------------------+
// |        JSPandaFile Size         |
// +---------------------------------+
// |        moduleName Length        |
// |           moduleName            |
// +---------------------------------+<-------- RecordInfoSection
// |      hasRecordInfoSection       |  // 1 byte: 0 = no recordInfo, 1 = has recordInfo
// +---------------------------------+
// |           numClasses_           |
// +---------------------------------+
// |       jsRecordInfo_ count       |
// +---------------------------------+
// |         JSRecordInfo[0]         |
// |  recordName (len + data)        |
// |  flags (isCjs, isJson, etc.)    |
// |  classId, moduleRecordIdx, etc. |
// |  npmPackageName (len + data)    |
// +---------------------------------+
// |              ...                |
// +---------------------------------+
// |         npmEntries_ count       |
// +---------------------------------+
// |           npmEntry[0]           |
// |       key (len + data)          |
// |       value (len + data)        |
// +---------------------------------+
// |              ...                |
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

// CrossBundleHsp snapshot layout
// +---------------------------------+<-------- FileHeader (SnapshotVersionInfo)
// |    Application Version Code     |
// +---------------------------------+
// |      System Version Length      |
// |      Description Length         |
// +---------------------------------+
// |      System Version Code        |
// |          Description            |
// |      [Padding] (optional)       |  // align up to sizeof(uint64_t)
// +---------------------------------+<-------- CrossBundleHspEntries
// |           pathCount             |
// +---------------------------------+
// |      CrossBundleHspEntry[0]     |
// |    hspPath (len + data)         |
// |    entryPoint (len + data)      |
// |    fileSize                     |
// |    checkSum                     |
// +---------------------------------+
// |              ...                |
// +---------------------------------+<-------- FileCheckSum
// |           FileCheckSum          |  // adler32 over all preceding content
// +---------------------------------+
    static bool WriteDataToFile(JSThread *thread, JSPandaFile *jsPandaFile, const CString &path,
        const CString &version);
    static bool ReadDataFromFile(JSThread *thread, JSPandaFile *jsPandaFile, const CString &path,
        const CString &version);
    static bool WriteCrossBundleHspDataToFile(JSThread *thread, const CString &path, const CString &version);
    class JSPandaFileSnapshotTask : public common::Task {
    public:
        JSPandaFileSnapshotTask(int32_t id, JSThread *thread, JSPandaFile *jsPandaFile, const CString &path,
            const CString &version) : Task(id), thread_(thread), jsPandaFile_(jsPandaFile), path_(path),
            version_(version) {}
        ~JSPandaFileSnapshotTask() override = default;
        bool Run(uint32_t threadIndex) override;

        NO_COPY_SEMANTIC(JSPandaFileSnapshotTask);
        NO_MOVE_SEMANTIC(JSPandaFileSnapshotTask);

    private:
        JSThread *thread_ {nullptr};
        JSPandaFile *jsPandaFile_ {nullptr};
        CString path_ {};
        CString version_ {};
    };
    class CrossBundleHspSnapshotTask : public common::Task {
    public:
        CrossBundleHspSnapshotTask(int32_t id, JSThread *thread, const CString &path, const CString &version)
            : Task(id), thread_(thread), path_(path), version_(version) {}
        ~CrossBundleHspSnapshotTask() override = default;
        bool Run(uint32_t threadIndex) override;

        NO_COPY_SEMANTIC(CrossBundleHspSnapshotTask);
        NO_MOVE_SEMANTIC(CrossBundleHspSnapshotTask);

    private:
        JSThread *thread_ {nullptr};
        CString path_ {};
        CString version_ {};
    };

    class CrossBundleHspEntry {
    public:
        CrossBundleHspEntry() = default;
        CrossBundleHspEntry(const CString &hspPath, const CString &entryPoint, uint32_t fileSize, uint32_t checkSum)
            : hspPath_(hspPath), entryPoint_(entryPoint), fileSize_(fileSize), checkSum_(checkSum) {}

        const CString &GetHspPath() const { return hspPath_; }
        const CString &GetEntryPoint() const { return entryPoint_; }
        uint32_t GetFileSize() const { return fileSize_; }
        uint32_t GetCheckSum() const { return checkSum_; }

        uint32_t SerializedSize() const
        {
            return sizeof(uint32_t) + hspPath_.size()
                 + sizeof(uint32_t) + entryPoint_.size()
                 + sizeof(uint32_t) + sizeof(uint32_t);
        }

        bool WriteTo(FileMemMapWriter &writer) const;
        bool ReadFrom(FileMemMapReader &reader);

    private:
        CString hspPath_;
        CString entryPoint_;
        uint32_t fileSize_ {0};
        uint32_t checkSum_ {0};
    };

    class ReadEscapeGuard {
    public:
        explicit ReadEscapeGuard(const CString &path) : path_(path) {}
        ~ReadEscapeGuard()
        {
            if (!done_) {
                ModulesSnapshotHelper::TryDisableSnapshotOnANR();
                ModulesSnapshotHelper::UpdateFromStateFile(path_);
            }
        }
        NO_COPY_SEMANTIC(ReadEscapeGuard);
        NO_MOVE_SEMANTIC(ReadEscapeGuard);

        void Done() { done_ = true; }

    private:
        CString path_;
        bool done_ {false};
    };
};
}  // namespace panda::ecmascript
#endif // ECMASCRIPT_PANDAFILE_JS_PANDAFILE_SNAPSHOT_H