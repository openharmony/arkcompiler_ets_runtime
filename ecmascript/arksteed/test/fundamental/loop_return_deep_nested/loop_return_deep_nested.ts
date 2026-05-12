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
function loop_return_deep_nested(a, b, c, target_i, target_j, target_k)
{
    let sum = 0;
    for (let i = 0; i < a; i++) {
        for (let j = 0; j < b; j++) {
            for (let k = 0; k < c; k++) {
                sum += (i + 1) * (j + 1) * (k + 1);
                if (i == target_i && j == target_j && k == target_k) {
                    return i * 100 + j * 10 + k;
                }
                sum += (i + 1) + (j + 1) + (k + 1);
            }
        }
    }
    return sum;
}

ArkTools.arkSteedCompileAsync(loop_return_deep_nested);

let time = Date.now();
for (let cur = Date.now(); cur - time < 1000; cur = Date.now()) {}

print(loop_return_deep_nested(5, 5, 5, 1, 1, 1));
print(loop_return_deep_nested(2, 4, 6, 1, 3, 5));
print(loop_return_deep_nested(4, 3, 7, 3, 2, 6));
