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

#ifndef MAPLE_IR_INCLUDE_INTRINSIC_OP_H
#define MAPLE_IR_INCLUDE_INTRINSIC_OP_H

namespace maple {
enum MIRIntrinsicID {
#define DEF_MIR_INTRINSIC(STR, NAME, RETURN_TYPE, ...) INTRN_##STR,
#include "intrinsics.def"
#undef DEF_MIR_INTRINSIC
};
}  // namespace maple
#endif  // MAPLE_IR_INCLUDE_INTRINSIC_OP_H
