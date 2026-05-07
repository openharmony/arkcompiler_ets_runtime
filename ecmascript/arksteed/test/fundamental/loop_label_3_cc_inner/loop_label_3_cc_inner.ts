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
function loop_label_3_cc_inner(limit, c_inner) {
    let sum = 0;
    let outerCount = 0;
    let midCount = 0;
    let innerCount = 0;

    outerLoop: for (let i = 0; i < limit; i++) {
        outerCount++;
        midLoop: for (let j = 0; j < limit; j++) {
            midCount++;
            innerLoop: for (let k = 0; k < limit; k++) {
                innerCount++;
                if (k % c_inner === 1) {
                    continue innerLoop;
                }
                sum += i + j + k;
            }
        }
    }

    return sum * 1000 + outerCount * 100 + midCount * 10 + innerCount;
}

ArkTools.arkSteedCompileAsync(loop_label_3_cc_inner);

let time = Date.now();
for (let cur = Date.now(); cur - time < 1000; cur = Date.now()) {}

print(loop_label_3_cc_inner(0, 2));
print(loop_label_3_cc_inner(2, 3));
print(loop_label_3_cc_inner(4, 2));
print(loop_label_3_cc_inner(6, 4));
print(loop_label_3_cc_inner(8, 3));
print(loop_label_3_cc_inner(10, 2));
print(loop_label_3_cc_inner(12, 6));
print(loop_label_3_cc_inner(14, 5));
print(loop_label_3_cc_inner(16, 7));
print(loop_label_3_cc_inner(18, 4));
