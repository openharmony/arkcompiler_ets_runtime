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
function loop_label_3_cb_outer(limit, c_outer, b_inner) {
    let sum = 0;
    let outerCount = 0;
    let middleCount = 0;
    let innerCount = 0;

    outerLoop: for (let i = 0; i < limit; i++) {
        outerCount++;
        middleLoop: for (let j = 0; j < limit; j++) {
            middleCount++;
            innerLoop: for (let k = 0; k < limit; k++) {
                innerCount++;
                if (i % c_outer === 1) {
                    continue outerLoop;
                }
                if (k % b_inner === 2) {
                    break innerLoop;
                }
                sum += i + j + k;
            }
        }
    }

    return sum * 1000 + outerCount * 100 + middleCount * 10 + innerCount;
}

ArkTools.arkSteedCompileAsync(loop_label_3_cb_outer);

let time = Date.now();
for (let cur = Date.now(); cur - time < 1000; cur = Date.now()) {}

print(loop_label_3_cb_outer(0, 2, 3));
print(loop_label_3_cb_outer(2, 3, 3));
print(loop_label_3_cb_outer(4, 2, 4));
print(loop_label_3_cb_outer(6, 4, 5));
print(loop_label_3_cb_outer(8, 2, 3));
print(loop_label_3_cb_outer(10, 3, 6));
print(loop_label_3_cb_outer(12, 5, 7));
print(loop_label_3_cb_outer(14, 4, 3));
print(loop_label_3_cb_outer(16, 6, 8));
print(loop_label_3_cb_outer(18, 3, 9));
