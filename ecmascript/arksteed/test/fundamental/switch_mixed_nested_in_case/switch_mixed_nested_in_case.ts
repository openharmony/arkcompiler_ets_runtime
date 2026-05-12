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
function switch_mixed_nested_in_case(x, y, a, b, c, d)
{
    let result = a;
    switch (x) {
        case 1:
            {
                let inner = 0;
                switch (y) {
                    case 1:
                        inner = b * 2;
                        break;
                    case 2:
                        inner = b + 10;
                        break;
                    default:
                        inner = b;
                        break;
                }
                result = result + inner;
            }
            break;
        case 2:
            {
                let inner = 0;
                switch (y) {
                    case 1:
                        inner = c * 3;
                        break;
                    default:
                        inner = c + 5;
                        break;
                }
                result = result + inner;
            }
            break;
        default:
            result = result + d;
            break;
    }
    return result * 10;
}

ArkTools.arkSteedCompileAsync(switch_mixed_nested_in_case);

let time = Date.now();
for (let cur = Date.now(); cur - time < 1000; cur = Date.now()) {}

print(switch_mixed_nested_in_case(1, 1, 10, 5, 3, 7));
print(switch_mixed_nested_in_case(1, 2, 10, 5, 3, 7));
print(switch_mixed_nested_in_case(2, 1, 10, 5, 3, 7));
print(switch_mixed_nested_in_case(3, 1, 10, 5, 3, 7));