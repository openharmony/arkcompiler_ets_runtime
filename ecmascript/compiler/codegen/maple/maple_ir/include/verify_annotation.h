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

#ifndef MAPLEIR_VERIFY_ANNOTATION_H
#define MAPLEIR_VERIFY_ANNOTATION_H
#include "mir_module.h"
#include "mir_type.h"
#include "verify_pragma_info.h"

namespace maple {
void AddVerfAnnoThrowVerifyError(MIRModule &md, const ThrowVerifyErrorPragma &info, MIRStructType &clsType);
void AddVerfAnnoAssignableCheck(MIRModule &md, std::vector<const AssignableCheckPragma *> &info,
                                MIRStructType &clsType);
void AddVerfAnnoExtendFinalCheck(MIRModule &md, MIRStructType &clsType);
void AddVerfAnnoOverrideFinalCheck(MIRModule &md, MIRStructType &clsType);
}  // namespace maple
#endif  // MAPLEALL_VERIFY_ANNOTATION_H