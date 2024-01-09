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

#include <sstream>

namespace maple {
std::string GetNthStr(size_t index)
{
    switch (index) {
        case 0:
            return "1st";
        case 1:
            return "2nd";
        case 2: // 2 is third index from 0
            return "3rd";
        default: {
            std::ostringstream oss;
            oss << (index + 1) << "th";
            return oss.str();
        }
    }
}
}  // namespace maple
