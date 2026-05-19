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
function loop_for_4(arr, limit, p, q, r)
{
    for(var i = 0; i < limit; i++){
        arr.push(i * p * q + r)
    }
    for(var i = 0; i < limit; i++){
        arr.push(i * q * r + p);
    }
}

function getSum(arr: number[])
{
    let sum = 0;
    for (let value of arr) {
        sum += value;
    }
    return sum;
}

ArkTools.arkSteedCompileSync(loop_for_4);

// TODO: Replace the spin loop with:
// let res = ArkTools.waitJitCompileFinish(loop_for_3);
// print(res);

let arr = [1, 2, 3];
loop_for_4(arr, 0, 3, 1, 5);
print(getSum(arr));
loop_for_4(arr, 5, 7, 2, 3);
print(getSum(arr));
loop_for_4(arr, 10, 11, 3, 7);
print(getSum(arr));
