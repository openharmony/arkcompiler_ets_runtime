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

#ifndef ADAPTER_OHOS_EXTRACTORTOOL_EXTRACTOR_ADAPTER_H
#define ADAPTER_OHOS_EXTRACTORTOOL_EXTRACTOR_ADAPTER_H

#include <memory>
#include <string>
#include "extractor.h"
#include "file_mapper.h"
#include "zip_file.h"

namespace panda::ecmascript {

// Bridge type aliases: consumer code keeps using panda::ecmascript::Extractor etc.
using Extractor = OHOS::AbilityBase::Extractor;
using ExtractorUtil = OHOS::AbilityBase::ExtractorUtil;
using ZipFile = OHOS::AbilityBase::ZipFile;
using FileMapper = OHOS::AbilityBase::FileMapper;
using FileInfo = OHOS::AbilityBase::FileInfo;
using FileMapperType = OHOS::AbilityBase::FileMapperType;
using ZipEntryMap = OHOS::AbilityBase::ZipEntryMap;
using ZipPos = OHOS::AbilityBase::ZipPos;

// Bridge: foundation GetSafeData returns unique_ptr, arkcompiler consumers hold shared_ptr
std::shared_ptr<FileMapper> GetSafeDataAsShared(Extractor& extractor, const std::string& fileName);

// Bridge: foundation has no GetFilePathByOffset, implement via ZipFile::GetAllEntries + GetDataOffsetRelative
std::string GetFilePathByOffset(ZipFile& zipFile, uintptr_t offset);

}  // namespace panda::ecmascript
#endif  // ADAPTER_OHOS_EXTRACTORTOOL_EXTRACTOR_ADAPTER_H
