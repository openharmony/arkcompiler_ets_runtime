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
#include "ecmascript/ohos/module_snapshot_interfaces.h"

#include "ecmascript/module/module_snapshot.h"
#include "ecmascript/ohos/ohos_version_info_tools.h"
#include "ecmascript/platform/filesystem.h"
#include "ecmascript/snapshot/common/modules_snapshot_helper.h"

namespace panda::ecmascript::ohos {

void ModuleSnapshotInterfaces::Serialize(const EcmaVM *vm, const CString &path)
{
    LOG_ECMA(DEBUG) << "ModuleSnapshotInterfaces::Serialize: " << path;
    ECMA_BYTRACE_NAME(HITRACE_LEVEL_COMMERCIAL, HITRACE_TAG_ARK, "ModuleSnapshotInterfaces::Serialize", "");
    if (ModulesSnapshotHelper::IsModuleSnapshotDisabled(path)) {
        LOG_ECMA(DEBUG) << "Serialize: Module Snapshot is not enabled";
        auto dataFile = base::ConcatToCString(path, ModuleSnapshot::MODULE_SNAPSHOT_FILE_NAME);
        if (FileExist(dataFile.c_str())) {
            Unlink(dataFile.c_str());
        }
        return;
    }
    CString version = OhosVersionInfoTools::GetRomVersion();
    if (version.empty()) {
        LOG_ECMA(ERROR) << "ModuleSnapshotInterface::Serialize rom version is empty";
        return;
    }
    ModulesSnapshotHelper::MarkModuleSnapshotLoaded();
    ModuleSnapshot::SerializeDataAndPostSavingJob(vm, path, version);
}

void ModuleSnapshotInterfaces::Deserialize(const EcmaVM *vm, const CString &path)
{
    LOG_ECMA(DEBUG) << "ModuleSnapshotInterfaces::Deserialize: " << path;
    ECMA_BYTRACE_NAME(HITRACE_LEVEL_COMMERCIAL, HITRACE_TAG_ARK, "ModuleSnapshotInterfaces::Deserialize", "");
    if (ModulesSnapshotHelper::IsModuleSnapshotDisabled(path)) {
        LOG_ECMA(DEBUG) << "DeserializeData: Module snapshot not enabled";
        return;
    }
    CString version = OhosVersionInfoTools::GetRomVersion();
    if (version.empty()) {
        LOG_ECMA(ERROR) << "ModuleSnapshotInterface::Serialize rom version is empty";
        return;
    }
    ModuleSnapshot::DeserializeData(vm, path, version);
}
} // namespace panda::ecmascript::ohos