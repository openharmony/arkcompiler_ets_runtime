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

function sumArrayRecursive(arr: number[], index: number): number {
    if (index >= arr.length) return 0;
    return arr[index] + sumArrayRecursive(arr, index + 1);
}

function loop_for_in_with_recursion(obj: Record<string, number[]>): number {
    let total = 0;
    for (const key in obj) {
        let arr = obj[key];
        total += sumArrayRecursive(arr, 0);
    }
    return total;
}

ArkTools.arkSteedCompileSync(loop_for_in_with_recursion);


print(loop_for_in_with_recursion({a: [1, 2, 3], b: [4, 5]}));
print(loop_for_in_with_recursion({x: [10]}));
print(loop_for_in_with_recursion({}));
print(loop_for_in_with_recursion({a: [], b: [1]}));
