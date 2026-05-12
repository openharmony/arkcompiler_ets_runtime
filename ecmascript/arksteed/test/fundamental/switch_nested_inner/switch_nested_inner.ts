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
function switch_nested_inner(x, y, a, b, c, d)
{
    let base = a * 10;
    let inner = 0;
    switch (x) {
        case 1:
            switch (y) {
                case 1:
                    inner = b + 10;
                    break;
                case 2:
                    inner = b - 5;
                    break;
                default:
                    inner = b * 2;
                    break;
            }
            return base + inner;
        case 2:
            switch (y) {
                case 1:
                    inner = c + 20;
                    break;
                default:
                    inner = c - 10;
                    break;
            }
            return base - inner;
        default:
            return base + d;
    }
}

ArkTools.arkSteedCompileAsync(switch_nested_inner);

let time = Date.now();
for (let cur = Date.now(); cur - time < 1000; cur = Date.now()) {}

print(switch_nested_inner(1, 1, 10, 5, 3, 7));
print(switch_nested_inner(1, 2, 10, 5, 3, 7));
print(switch_nested_inner(1, 5, 10, 5, 3, 7));
print(switch_nested_inner(2, 1, 10, 5, 3, 7));
print(switch_nested_inner(2, 3, 10, 5, 3, 7));
print(switch_nested_inner(5, 1, 10, 5, 3, 7));