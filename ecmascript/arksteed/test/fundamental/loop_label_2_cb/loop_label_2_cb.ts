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
// Labeled continue outer, labeled break inner
function loop_label_2_cb(limit, b1, b2) {
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
                break innerLoop;
            }
            sum += (i + 1) * (j + 1);
        }
    }

    return sum * 100 + outerCount * 10 + innerCount;
}

ArkTools.arkSteedCompileSync(loop_label_2_cb);


print(loop_label_2_cb(0, 2, 3));
print(loop_label_2_cb(2, 3, 4));
print(loop_label_2_cb(4, 4, 5));
print(loop_label_2_cb(6, 2, 3));
print(loop_label_2_cb(8, 5, 6));
print(loop_label_2_cb(10, 2, 3));
print(loop_label_2_cb(12, 6, 4));
print(loop_label_2_cb(14, 4, 7));
print(loop_label_2_cb(16, 2, 4));
print(loop_label_2_cb(18, 3, 3));
print(loop_label_2_cb(20, 5, 4));
