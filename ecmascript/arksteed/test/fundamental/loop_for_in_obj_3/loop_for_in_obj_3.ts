/*
 * Copyright (c) 2022 Huawei Device Co., Ltd.
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed in in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

declare function print(str: number):string;

function loop_for_in_obj_3(obj: Record<string, number>): number {
    let a = 0;
    let b = 0;
    let c = 0;
    for (let key in obj) {
        if (key.length % 3 === 0) {
            a += 1;
        } else if (key.length % 3 === 1) {
            b += 1;
        } else {
            c += 1;
        }
    }
    return a * 100 + b * 10 + c;
}

ArkTools.arkSteedCompileAsync(loop_for_in_obj_3);

let time = Date.now();
for (let cur = Date.now(); cur - time < 1000; cur = Date.now()) {}

print(loop_for_in_obj_3({}));
print(loop_for_in_obj_3({"first": 1, "second": 2}));
print(loop_for_in_obj_3({"one": 1, "two": 2, "three": 3, "four": 4}));
