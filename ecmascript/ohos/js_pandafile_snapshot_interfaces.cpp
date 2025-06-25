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
#include "ecmascript/ohos/js_pandafile_snapshot_interfaces.h"

#include "ecmascript/ohos/ohos_version_info_tools.h"
#include "ecmascript/jspandafile/js_pandafile_snapshot.h"
#include "ecmascript/platform/filesystem.h"

namespace panda::ecmascript::ohos {

void JSPandaFileSnapshotInterfaces::Serialize(const EcmaVM *vm, const CString &path)
{
    LOG_ECMA(INFO) << "PandaFileSnapshotInterfaces::Serialize: " << path;
    ECMA_BYTRACE_NAME(HITRACE_LEVEL_COMMERCIAL, HITRACE_TAG_ARK, "PandaFileSnapshotInterfaces::Serialize", "");
    // check application white list
    if (!filesystem::Exists(path.c_str())) {
        LOG_ECMA(INFO) << "PandaFileSnapshotInterfaces::Serialize: " << path << " is not exists";
        return;
    }
    CString version = OhosVersionInfoTools::GetRomVersion();
    if (version.empty()) {
        LOG_ECMA(ERROR) << "PandaFileSnapshotInterfaces::Serialize rom version is empty";
        return;
    }
    JSPandaFileSnapshot::PostWriteDataToFileJob(vm, path, version);
}
} // namespace panda::ecmascript