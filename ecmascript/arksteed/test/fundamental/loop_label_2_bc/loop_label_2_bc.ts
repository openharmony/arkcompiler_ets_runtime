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
// Labeled break outer, labeled continue inner
function loop_label_2_bc(limit, b1, b2) {
    let sum = 0;
    let outerCount = 0;
    let innerCount = 0;

    outerLoop: for (let i = 0; i < limit; i++) {
        outerCount++;
        innerLoop: for (let j = 0; j < limit; j++) {
            innerCount++;
            if (i % b1 === 1) {
                break outerLoop;
            }
            if (j % b2 === 2) {
                continue innerLoop;
            }
            sum += (i + 1) * (j + 1);
        }
    }

    return sum * 100 + outerCount * 10 + innerCount;
}

ArkTools.arkSteedCompileAsync(loop_label_2_bc);

let time = Date.now();
for (let cur = Date.now(); cur - time < 1000; cur = Date.now()) {}

print(loop_label_2_bc(0, 2, 3));
print(loop_label_2_bc(2, 3, 4));
print(loop_label_2_bc(4, 4, 5));
print(loop_label_2_bc(6, 2, 6));
print(loop_label_2_bc(8, 3, 7));
print(loop_label_2_bc(10, 5, 3));
print(loop_label_2_bc(12, 4, 4));
print(loop_label_2_bc(14, 2, 3));
print(loop_label_2_bc(16, 3, 5));
print(loop_label_2_bc(18, 4, 4));
print(loop_label_2_bc(20, 5, 4));
