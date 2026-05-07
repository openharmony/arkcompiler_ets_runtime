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
// Labeled continue on both outer and inner loops
function loop_label_2_cc(limit, b1, b2) {
    let sum = 0;
    let outerCount = 0;
    let innerCount = 0;

    outerLoop: for (let i = 0; i < limit; i++) {
        outerCount++;
        innerLoop: for (let j = 0; j < limit; j++) {
            innerCount++;
            if (i % b1 === 1) {
                continue outerLoop;
            }
            if (j % b2 === 2) {
                continue innerLoop;
            }
            sum += (i + 1) * (j + 1);
        }
    }

    return sum * 100 + outerCount * 10 + innerCount;
}

ArkTools.arkSteedCompileAsync(loop_label_2_cc);

let time = Date.now();
for (let cur = Date.now(); cur - time < 1000; cur = Date.now()) {}

print(loop_label_2_cc(0, 2, 3));
print(loop_label_2_cc(2, 3, 4));
print(loop_label_2_cc(4, 4, 3));
print(loop_label_2_cc(6, 2, 5));
print(loop_label_2_cc(8, 5, 6));
print(loop_label_2_cc(10, 2, 3));
print(loop_label_2_cc(12, 6, 4));
print(loop_label_2_cc(14, 4, 7));
print(loop_label_2_cc(16, 2, 3));
print(loop_label_2_cc(18, 3, 8));
print(loop_label_2_cc(20, 5, 4));
