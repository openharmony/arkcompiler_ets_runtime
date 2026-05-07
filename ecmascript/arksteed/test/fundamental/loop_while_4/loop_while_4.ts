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

function calculateValue(x: number, y: number): number {
    if (x > y) {
        return x * y;
    } else if (x < y) {
        return x + y;
    } else {
        return x * x;
    }
}

function loop_while_4(n: number): number {
    let sum = 0;
    let i = 0;
    while (i < n) {
        let j = 0;
        while (j < n) {
            let k = 0;
            while (k < 5) {
                if ((i + j + k) % 3 === 0) {
                    sum += calculateValue(i, j);
                }
                k++;
            }
            j++;
        }
        i++;
    }
    return sum;
}

ArkTools.arkSteedCompileAsync(loop_while_4);
let time = Date.now();
for (let cur = Date.now(); cur - time < 1000; cur = Date.now()) {}

let ret = loop_while_4(10);
print(ret);