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

function loop_for_in_with_try_catch(arr: number[]): number {
    let sum = 0;
    for (const key in arr) {
        try {
            let val = arr[key];
            if (val < 0) {
                throw "negative";
            }
            sum += val;
        } catch (e) {
            sum += 0;
        }
    }
    return sum;
}

ArkTools.arkSteedCompileSync(loop_for_in_with_try_catch);


print(loop_for_in_with_try_catch([1, 2, 3]));
print(loop_for_in_with_try_catch([5, -1, 3]));
print(loop_for_in_with_try_catch([-1, -2]));
print(loop_for_in_with_try_catch([]));
