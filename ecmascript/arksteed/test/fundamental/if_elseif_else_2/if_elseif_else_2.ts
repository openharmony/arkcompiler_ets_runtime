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
function if_elseif_else_2(x, a, b, c)
{
    let result = 0;

    if (x > 20) {
        result = a * b + c;
    } else if (x > 10) {
        result = a + b + c;
    } else {
        result = a - b - c;
    }

    return result;
}

ArkTools.arkSteedCompileAsync(if_elseif_else_2);

// TODO: Replace the spin loop with:
// let res = ArkTools.waitJitCompileFinish(if_elseif_else_2);
// print(res);
let time = Date.now();
for (let cur = Date.now(); cur - time < 1000; cur = Date.now()) {}

let output = if_elseif_else_2(5, 3, 5, 7);
print(output);
output = if_elseif_else_2(15, 3, 5, 7);
print(output);
output = if_elseif_else_2(25, 3, 5, 7);
print(output);
