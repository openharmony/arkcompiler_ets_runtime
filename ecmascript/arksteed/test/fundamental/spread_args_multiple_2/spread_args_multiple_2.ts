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
function spread_args_multiple_helper(...args: number[]): number {
    let sum = 0;
    let base = 1;
    for (let i = 0; i < args.length; i++) {
        sum += args[i] * base;
        base *= 10;
    }
    return sum;
}

function spread_args_multiple_2(first: number[], second: number[]): number {
    return spread_args_multiple_helper(...first, 3, ...second);
}

ArkTools.arkSteedCompileSync(spread_args_multiple_helper);
ArkTools.arkSteedCompileSync(spread_args_multiple_2);


print(spread_args_multiple_2([1, 2], [4, 5, 6, 7, 8]));
