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
#include "constpool_snapshot_interfaces.h"

#include "ecmascript/jspandafile/constpool_snapshot.h"
#include "ecmascript/jspandafile/js_pandafile_manager.h"
#include "ecmascript/platform/filesystem.h"
#include "ecmascript/snapshot/common/modules_snapshot_helper.h"
#include "ohos_version_info_tools.h"

namespace panda::ecmascript::ohos {

void ConstPoolSnapshotInterfaces::Serialize(const EcmaVM *vm, const CString &path)
{
    LOG_ECMA(DEBUG) << "ConstPoolSnapshotInterfaces::Serialize: " << path;
    ECMA_BYTRACE_NAME(HITRACE_LEVEL_COMMERCIAL, HITRACE_TAG_ARK, "ConstPoolSnapshotInterfaces::Serialize", "");
    // follow policy of module snapshot
    if (ModulesSnapshotHelper::IsConstPoolSnapshotDisabled(path)) {
        LOG_ECMA(WARN) << "Serialize: ConstPool Snapshot is not enabled";
        ModulesSnapshotHelper::HandleCorruptedFile(path, CString(ConstPoolSnapshot::CONSTPOOL_SNAPSHOT_FILE_SUFFIX));
        return;
    }
    if (!ecmascript::Runtime::GetInstance()->IsMainProcess()) {
        LOG_ECMA(WARN) << "ConstPoolSnapshotInterfaces::Serialize skiped in child process";
        return;
    }
    CString version = OhosVersionInfoTools::GetRomVersion();
    if (version.empty()) {
        LOG_ECMA(DEBUG) << "ConstPoolSnapshotInterfaces::Serialize rom version is empty";
        return;
    }
    ModulesSnapshotHelper::MarkConstPoolSnapshotLoaded();
    auto pandafiles = JSPandaFileManager::GetInstance()->GetAppJSPandaFiles(vm, true);
    for (auto pf : pandafiles) {
        ConstPoolSnapshot::SerializeDataAndPostSavingJob(vm, pf.get(), path, version);
    }
}
} // namespace panda::ecmascript::ohos
