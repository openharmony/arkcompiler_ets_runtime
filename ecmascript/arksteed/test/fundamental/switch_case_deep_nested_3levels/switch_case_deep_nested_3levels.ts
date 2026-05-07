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
function switch_case_deep_nested_3levels(x, y, z, a, b, c, d)
{
    let result = a;
    switch (x) {
        case 1:
            result = result + 10;
            switch (y) {
                case 1:
                    result = result + 5;
                    switch (z) {
                        case 1:
                            return result + b;
                        case 2:
                            return result - b;
                        default:
                            return result * 2;
                    }
                case 2:
                    result = result - 5;
                    switch (z) {
                        case 1:
                            return result + c;
                        default:
                            return result - c;
                    }
                default:
                    result = result * 2;
                    return result + d;
            }
        case 2:
            result = result * 2;
            switch (y) {
                case 1:
                    switch (z) {
                        case 1:
                            return result + b;
                        default:
                            return result + c;
                    }
                default:
                    return result + d;
            }
        default:
            return result + a + b + c + d;
    }
}

ArkTools.arkSteedCompileAsync(switch_case_deep_nested_3levels);

let time = Date.now();
for (let cur = Date.now(); cur - time < 1000; cur = Date.now()) {}

print(switch_case_deep_nested_3levels(1, 1, 1, 10, 5, 3, 7));
print(switch_case_deep_nested_3levels(1, 1, 2, 10, 5, 3, 7));
print(switch_case_deep_nested_3levels(1, 1, 5, 10, 5, 3, 7));
print(switch_case_deep_nested_3levels(1, 2, 1, 10, 5, 3, 7));
print(switch_case_deep_nested_3levels(1, 2, 5, 10, 5, 3, 7));
print(switch_case_deep_nested_3levels(1, 5, 1, 10, 5, 3, 7));
print(switch_case_deep_nested_3levels(2, 1, 1, 10, 5, 3, 7));
print(switch_case_deep_nested_3levels(2, 3, 5, 10, 5, 3, 7));
print(switch_case_deep_nested_3levels(5, 1, 1, 10, 5, 3, 7));