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
function loop_return_inner_only(a, b, offset_i, offset_j)
{
    let sum = 0;
    for (let i = 0; i < a; i++) {
        sum += (i + offset_i) * 10;
        for (let j = 0; j < b; j++) {
            sum += (j + offset_j);
            if (j == 2) {
                return (j + offset_j) * 10;
            }
            sum += (j + offset_j) * 5;
        }
    }
    return sum;
}

ArkTools.arkSteedCompileAsync(loop_return_inner_only);

let time = Date.now();
for (let cur = Date.now(); cur - time < 1000; cur = Date.now()) {}

print(loop_return_inner_only(5, 8, 1, 2));
print(loop_return_inner_only(3, 2, 0, 0));
print(loop_return_inner_only(6, 10, 3, 5));
