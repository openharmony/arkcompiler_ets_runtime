/*
 * Copyright (c) 2024 Huawei Device Co., Ltd.
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

function loop_for_of_set(n: number): number {
    let set = new Set([1, 2, 3]);
    let sum = 0;
    for (let i = 0; i < n; ++i) {
        for (let key of set.keys()) {
            sum += key;
        }
    } 
    return sum;
}

ArkTools.arkSteedCompileAsync(loop_for_of_set);
let time = Date.now();
for (let cur = Date.now(); cur - time < 1000; cur = Date.now()) {}

let ret = loop_for_of_set(2000);
print(ret);
