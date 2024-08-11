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

#ifndef MAPLEBE_INCLUDE_AD_MAD_H
#define MAPLEBE_INCLUDE_AD_MAD_H
#include <vector>
#include <bitset>
#include "types_def.h"
#include "mpl_logging.h"
#include "insn.h"

namespace maplebe {
enum UnitType : maple::uint8 { kUnitTypePrimart, kUnitTypeOr, kUnitTypeAnd, KUnitTypeNone };


/* machine model */
enum LatencyType : maple::uint32 {
/* LT: latency */
#include "mplad_latency_type.def"
    kLtLast,
};
} /* namespace maplebe */

#endif /* MAPLEBE_INCLUDE_AD_MAD_H */
