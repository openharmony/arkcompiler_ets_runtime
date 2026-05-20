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

#ifndef ECMASCRIPT_JSPANDAFILE_CONSTPOOL_SNAPSHOT_H
#define ECMASCRIPT_JSPANDAFILE_CONSTPOOL_SNAPSHOT_H

#include "common_components/taskpool/taskpool.h"
#include "ecmascript/js_handle.h"
#include "ecmascript/mem/c_string.h"
#include "ecmascript/serializer/serialize_data.h"

#include <memory>

namespace panda::ecmascript {
class EcmaVM;

class ConstPoolSnapshot {
public:
    static constexpr std::string_view CONSTPOOL_SNAPSHOT_FILE_SUFFIX = "_ConstPool.ams";

    static void SerializeDataAndPostSavingJob(const EcmaVM* vm, JSPandaFile* pandafile, const CString& path,
                                              const CString& version);
    static bool DeserializeData(EcmaVM* vm, JSPandaFile* pandafile, const CString& path, const CString& version);

private:
    // ConstPool snapshot layout
    // +--------------------------------------+<-------- BaseInfo
    // |        Snapshot Version Info         | @see: ModulesSnapshotHelper::SnapshotVersionInfo
    // +--------------------------------------+<-------- data
    // |               dataIndex              |
    // +--------------------------------------+
    // |               sizeLimit              |
    // +--------------------------------------+
    // |       SerializeDataSizeGroup         |
    // +--------------------------------------+<-------- GC
    // |     regionRemainSizeVectors size     |
    // |       regionRemainSizeVectors        |
    // +--------------------------------------+<-------- data
    // |                buffer                |
    // +--------------------------------------+<-------- CheckSum
    // |               CheckSum               |
    // +--------------------------------------+
    static JSHandle<JSTaggedValue> GetSerializeArray(JSThread* thread, JSPandaFile* pandafile);
    static std::unique_ptr<SerializeData> GetSerializeData(JSThread* thread, JSPandaFile* pandafile);
    static CString GetSnapshotFileName(const JSPandaFile* pandafile);
};
} // namespace panda::ecmascript
#endif // ECMASCRIPT_JSPANDAFILE_CONSTPOOL_SNAPSHOT_H
