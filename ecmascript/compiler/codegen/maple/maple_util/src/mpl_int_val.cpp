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

#include "mpl_int_val.h"

namespace maple {

std::ostream &operator<<(std::ostream &os, const IntVal &value)
{
#ifdef ARK_LITECG_DEBUG
    int64 val = value.GetExtValue();
    constexpr int64 valThreshold = 1024;

    if (val <= valThreshold) {
        os << val;
    } else {
        os << std::hex << "0x" << val << std::dec;
    }
#endif
    return os;
}

}  // namespace maple