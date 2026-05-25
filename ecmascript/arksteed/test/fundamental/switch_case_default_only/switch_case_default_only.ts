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
function switch_case_default_only(x, a, b, c)
{
    let result = a;
    switch (x) {
        case 1:
            result = result + 10;
            break;
        case 2:
            result = result * 2;
            break;
        default:
            return a + b * c;
    }
    return result;
}

ArkTools.arkSteedCompileSync(switch_case_default_only);


print(switch_case_default_only(1, 10, 3, 2));
print(switch_case_default_only(2, 10, 3, 2));
print(switch_case_default_only(5, 10, 3, 2));