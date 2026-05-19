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
function switch_deep_nested_3levels(x, y, z, a, b, c, d)
{
    let result = 0;
    switch (x) {
        case 1:
            switch (y) {
                case 1:
                    switch (z) {
                        case 1:
                            result = a + b;
                            break;
                        case 2:
                            result = a - b;
                            break;
                        default:
                            result = a * 2;
                            break;
                    }
                    break;
                case 2:
                    switch (z) {
                        case 1:
                            result = a + c;
                            break;
                        default:
                            result = a - c;
                            break;
                    }
                    break;
                default:
                    result = a * 3;
                    break;
            }
            break;
        case 2:
            switch (y) {
                case 1:
                    switch (z) {
                        case 1:
                            result = a + b + c;
                            break;
                        default:
                            result = a + d;
                            break;
                    }
                    break;
                default:
                    result = a * 4;
                    break;
            }
            break;
        default:
            result = a + b + c + d;
            break;
    }
    return result;
}

ArkTools.arkSteedCompileSync(switch_deep_nested_3levels);


print(switch_deep_nested_3levels(1, 1, 1, 10, 5, 3, 7));
print(switch_deep_nested_3levels(1, 1, 2, 10, 5, 3, 7));
print(switch_deep_nested_3levels(1, 1, 5, 10, 5, 3, 7));
print(switch_deep_nested_3levels(1, 2, 1, 10, 5, 3, 7));
print(switch_deep_nested_3levels(1, 2, 5, 10, 5, 3, 7));
print(switch_deep_nested_3levels(1, 5, 1, 10, 5, 3, 7));
print(switch_deep_nested_3levels(2, 1, 1, 10, 5, 3, 7));
print(switch_deep_nested_3levels(2, 3, 5, 10, 5, 3, 7));
print(switch_deep_nested_3levels(5, 1, 1, 10, 5, 3, 7));