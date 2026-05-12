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
// Labeled break on all 4 levels
function loop_label_4_bbbb(limit, b1, b2, b3, b4) {
    let sum = 0;
    let outerCount = 0;
    let mid1Count = 0;
    let mid2Count = 0;
    let innerCount = 0;

    outerLoop: for (let i = 0; i < limit; i++) {
        outerCount++;
        mid1Loop: for (let j = 0; j < limit; j++) {
            mid1Count++;
            mid2Loop: for (let k = 0; k < limit; k++) {
                mid2Count++;
                innerLoop: for (let l = 0; l < limit; l++) {
                    innerCount++;
                    if (i % b1 === 1) {
                        break outerLoop;
                    }
                    if (j % b2 === 1) {
                        break mid1Loop;
                    }
                    if (k % b3 === 1) {
                        break mid2Loop;
                    }
                    if (l % b4 === 1) {
                        break innerLoop;
                    }
                    sum += i + j + k + l;
                }
            }
        }
    }

    return sum * 10000 + outerCount * 1000 + mid1Count * 100 + mid2Count * 10 + innerCount;
}

ArkTools.arkSteedCompileAsync(loop_label_4_bbbb);

let time = Date.now();
for (let cur = Date.now(); cur - time < 1000; cur = Date.now()) {}

print(loop_label_4_bbbb(0, 2, 2, 2, 2));
print(loop_label_4_bbbb(2, 3, 2, 3, 2));
print(loop_label_4_bbbb(4, 4, 3, 2, 3));
print(loop_label_4_bbbb(6, 3, 3, 3, 3));
print(loop_label_4_bbbb(8, 5, 4, 4, 4));
print(loop_label_4_bbbb(10, 5, 4, 3, 2));
print(loop_label_4_bbbb(12, 6, 5, 5, 5));
print(loop_label_4_bbbb(14, 2, 2, 2, 2));
print(loop_label_4_bbbb(16, 3, 3, 3, 3));
print(loop_label_4_bbbb(18, 4, 3, 2, 6));
