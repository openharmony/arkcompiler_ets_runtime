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

#ifndef MAPLE_IR_INCLUDE_PRINTING_H
#define MAPLE_IR_INCLUDE_PRINTING_H
#include <string>
#include "types_def.h"

namespace maple {
void PrintIndentation(int32 indent);
void PrintString(const std::string &str);
}  // namespace maple
#endif  // MAPLE_IR_INCLUDE_PRINTING_H
