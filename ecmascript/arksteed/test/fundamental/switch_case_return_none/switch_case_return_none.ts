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
function switch_case_return_none(x, a, b, c)
{
    let result = a;
    switch (x) {
        case 1:
            result = result + 10;
            break;
        case 2:
            result = result * 2 + 5;
            break;
        case 3:
            result = result - b + c;
            break;
        default:
            result = result + 100;
            break;
    }
    return result;
}

ArkTools.arkSteedCompileAsync(switch_case_return_none);

let time = Date.now();
for (let cur = Date.now(); cur - time < 1000; cur = Date.now()) {}

print(switch_case_return_none(1, 10, 3, 2));
print(switch_case_return_none(2, 10, 3, 2));
print(switch_case_return_none(3, 10, 3, 2));
print(switch_case_return_none(5, 10, 3, 2));