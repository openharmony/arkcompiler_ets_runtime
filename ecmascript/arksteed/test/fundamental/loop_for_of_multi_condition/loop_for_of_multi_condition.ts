/*
 * Copyright (c) 2022 Huawei Device Co., Ltd.
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

declare function print(str: number):string;

function loop_for_of_multi_condition(arr: number[]): number {
    let sumPos = 0;
    let sumNeg = 0;
    let countZero = 0;
    for (const val of arr) {
        if (val > 0) {
            sumPos += val;
        } else if (val < 0) {
            sumNeg += val;
        } else {
            countZero += 1;
        }
    }
    return sumPos * 100 + sumNeg * 10 + countZero;
}

ArkTools.arkSteedCompileSync(loop_for_of_multi_condition);


print(loop_for_of_multi_condition([1, -2, 0, 3]));
print(loop_for_of_multi_condition([10, 20]));
print(loop_for_of_multi_condition([-5, -10]));
print(loop_for_of_multi_condition([0, 0, 0]));
