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

function loop_for_in_return(obj: {[key: string]: number}, target: number): number {
    for (const key in obj) {
        if (obj[key] === target) {
            return obj[key];
        }
    }
    return -1;
}

ArkTools.arkSteedCompileSync(loop_for_in_return);


const obj1: {[key: string]: number} = {"a": 1, "b": 2, "c": 3, "d": 4, "e": 5};
const obj2: {[key: string]: number} = {"x": 10, "y": 20, "z": 30};

print(loop_for_in_return(obj1, 3));
print(loop_for_in_return(obj1, 5));
print(loop_for_in_return(obj1, 100));
print(loop_for_in_return(obj2, 20));
print(loop_for_in_return(obj2, 40));
