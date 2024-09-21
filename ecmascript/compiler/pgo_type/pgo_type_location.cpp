/*
 * Copyright (c) 2024 Huawei Device Co., Ltd.
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

#include "ecmascript/compiler/pgo_type/pgo_type_location.h"
#include "ecmascript/jspandafile/js_pandafile.h"

namespace panda::ecmascript::kungfu {

PGOTypeLocation::PGOTypeLocation(const JSPandaFile *jsPandaFile, uint32_t methodOffset, int32_t bcIdx)
    : abcName_(jsPandaFile->GetNormalizedFileDesc()), methodOffset_(methodOffset), bcIdx_(bcIdx)
{}
}  // namespace panda::ecmascript::kungfu
