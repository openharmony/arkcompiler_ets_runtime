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
function switch_case_multi_case_nested(x, y, a, b, c, d)
{
    let result = a;
    switch (x) {
        case 1:
        case 2:
            result = result + b;
            switch (y) {
                case 1:
                case 2:
                    return result + c;
                default:
                    return result - c;
            }
        case 3:
        case 4:
        case 5:
            result = result + d;
            switch (y) {
                case 1:
                    return result * 2;
                default:
                    return result + a;
            }
        default:
            return result + b + c + d;
    }
}

ArkTools.arkSteedCompileAsync(switch_case_multi_case_nested);

let time = Date.now();
for (let cur = Date.now(); cur - time < 1000; cur = Date.now()) {}

print(switch_case_multi_case_nested(1, 1, 10, 5, 3, 7));
print(switch_case_multi_case_nested(1, 3, 10, 5, 3, 7));
print(switch_case_multi_case_nested(2, 1, 10, 5, 3, 7));
print(switch_case_multi_case_nested(3, 1, 10, 5, 3, 7));
print(switch_case_multi_case_nested(3, 5, 10, 5, 3, 7));
print(switch_case_multi_case_nested(5, 1, 10, 5, 3, 7));
print(switch_case_multi_case_nested(9, 1, 10, 5, 3, 7));