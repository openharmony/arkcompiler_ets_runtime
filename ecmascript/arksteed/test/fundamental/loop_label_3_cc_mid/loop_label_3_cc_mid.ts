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
function loop_label_3_cc_mid(limit, c_mid, c_inner) {
    let sum = 0;
    let outerCount = 0;
    let midCount = 0;
    let innerCount = 0;

    outerLoop:
    for (let i = 0; i < limit; i++) {
        outerCount++;
        midLoop:
        for (let j = 0; j < limit; j++) {
            innerLoop:
            for (let k = 0; k < limit; k++) {
                innerCount++;
                if (k % c_inner === 1) {
                    continue innerLoop;
                }
                sum += i + j + k;
            }
            if (j % c_mid === 2) {
                continue midLoop;
            }
            midCount++;
        }
    }

    return sum * 1000 + outerCount * 100 + midCount * 10 + innerCount;
}

ArkTools.arkSteedCompileSync(loop_label_3_cc_mid);


print(loop_label_3_cc_mid(0, 3, 2));
print(loop_label_3_cc_mid(2, 4, 3));
print(loop_label_3_cc_mid(4, 5, 4));
print(loop_label_3_cc_mid(6, 6, 5));
print(loop_label_3_cc_mid(8, 7, 3));
print(loop_label_3_cc_mid(10, 8, 6));
print(loop_label_3_cc_mid(12, 9, 7));
print(loop_label_3_cc_mid(14, 10, 3));
print(loop_label_3_cc_mid(16, 11, 8));
print(loop_label_3_cc_mid(18, 3, 2));
