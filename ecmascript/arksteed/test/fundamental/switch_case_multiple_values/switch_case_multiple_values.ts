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
function switch_case_multiple_values(x, a, b, c, d)
{
    let result = a;
    switch (x) {
        case 1:
        case 2:
            result = result + b;
            break;
        case 3:
        case 4:
        case 5:
            result = result + c * 2;
            break;
        default:
            result = result + d;
            break;
    }
    return result * 10;
}

ArkTools.arkSteedCompileAsync(switch_case_multiple_values);

let time = Date.now();
for (let cur = Date.now(); cur - time < 1000; cur = Date.now()) {}

print(switch_case_multiple_values(1, 10, 5, 3, 7));
print(switch_case_multiple_values(2, 10, 5, 3, 7));
print(switch_case_multiple_values(3, 10, 5, 3, 7));
print(switch_case_multiple_values(4, 10, 5, 3, 7));
print(switch_case_multiple_values(5, 10, 5, 3, 7));
print(switch_case_multiple_values(9, 10, 5, 3, 7));