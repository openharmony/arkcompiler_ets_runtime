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
function spread_args_mixed_helper(sep: string, ...nums: number[]): string {
    let result = "";
    for (let i = 0; i < nums.length; i++) {
        if (i > 0) result += sep;
        result += nums[i];
    }
    return result;
}

function spread_args_mixed(): string {
    let r = "";
    r += spread_args_mixed_helper("-", ...[10, 20, 30]);
    r += " , ";
    r += spread_args_mixed_helper("+", 100, ...[200], 300);
    return r;
}

ArkTools.arkSteedCompileAsync(spread_args_mixed_helper);
ArkTools.arkSteedCompileAsync(spread_args_mixed);

let time = Date.now();
for (let cur = Date.now(); cur - time < 1000; cur = Date.now()) {}

print(spread_args_mixed());
