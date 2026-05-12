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
function loop_label_3_cb_mid(limit, c_mid, b_inner) {
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
                if (j % c_mid === 1) {
                    continue middleLoop;
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

ArkTools.arkSteedCompileAsync(loop_label_3_cb_mid);

let time = Date.now();
for (let cur = Date.now(); cur - time < 1000; cur = Date.now()) {}

print(loop_label_3_cb_mid(0, 2, 3));
print(loop_label_3_cb_mid(2, 3, 3));
print(loop_label_3_cb_mid(4, 2, 4));
print(loop_label_3_cb_mid(6, 4, 5));
print(loop_label_3_cb_mid(8, 3, 3));
print(loop_label_3_cb_mid(10, 3, 6));
print(loop_label_3_cb_mid(12, 5, 7));
print(loop_label_3_cb_mid(14, 4, 3));
print(loop_label_3_cb_mid(16, 6, 8));
print(loop_label_3_cb_mid(18, 2, 9));
