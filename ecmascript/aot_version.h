/*
 * Copyright (c) 2023 Huawei Device Co., Ltd.
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
#ifndef ECMASCRIPT_AOT_VERSION_H
#define ECMASCRIPT_AOT_VERSION_H

#include "ecmascript/base/file_header.h"

namespace panda::ecmascript {
class AOTFileVersion {
public:
    static constexpr base::FileHeader::VersionType AN_VERSION = {0, 0, 0, 2};
    static constexpr base::FileHeader::VersionType REWRITE_RELOCATE_AN_VERSION = {0, 0, 0, 1};
    static constexpr base::FileHeader::VersionType AI_VERSION = {0, 0, 0, 1};
};
} // namespace panda::ecmascript
#endif // ECMASCRIPT_AOT_VERSION_H
