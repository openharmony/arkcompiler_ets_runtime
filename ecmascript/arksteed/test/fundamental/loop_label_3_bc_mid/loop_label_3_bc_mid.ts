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
function loop_label_3_bc_mid(limit, b_inner, c_mid) {
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
                if (k % b_inner === 1) {
                    break innerLoop;
                }
                if (j % c_mid === 2) {
                    continue middleLoop;
                }
                sum += i + j + k;
            }
        }
    }

    return sum * 1000 + outerCount * 100 + middleCount * 10 + innerCount;
}

ArkTools.arkSteedCompileSync(loop_label_3_bc_mid);


print(loop_label_3_bc_mid(0, 2, 3));
print(loop_label_3_bc_mid(2, 3, 3));
print(loop_label_3_bc_mid(4, 2, 4));
print(loop_label_3_bc_mid(6, 4, 5));
print(loop_label_3_bc_mid(8, 3, 6));
print(loop_label_3_bc_mid(10, 2, 3));
print(loop_label_3_bc_mid(12, 5, 7));
print(loop_label_3_bc_mid(14, 3, 4));
print(loop_label_3_bc_mid(16, 6, 8));
print(loop_label_3_bc_mid(18, 7, 9));
