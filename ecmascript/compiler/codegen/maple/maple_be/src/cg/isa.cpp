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

#include "isa.h"
namespace maplebe {
#define DEFINE_MOP(op, ...) const OpndDesc OpndDesc::op = __VA_ARGS__;
#include "operand.def"
#undef DEFINE_MOP
#define DEFINE_MOP(op, ...) {abstract::op, __VA_ARGS__},
const InsnDesc InsnDesc::abstractId[abstract::kMopLast] = {
#include "abstract_mmir.def"
};
#undef DEFINE_MOP

bool InsnDesc::IsSame(const InsnDesc &left, std::function<bool(const InsnDesc &left, const InsnDesc &right)> cmp) const
{
    return cmp == nullptr ? false : cmp(left, *this);
}
}  // namespace maplebe
