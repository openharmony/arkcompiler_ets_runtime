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
let globalValue = 42;

function if_elseif_else_3(x, a, b, c)
{
    if (x > 20) {
        globalValue = a;
    } else if (x > 10) {
        b += globalValue;
    } else {
        globalValue += a;
        c += globalValue;
    }
    return a * 1000000 + b * 1000 + c;
}

ArkTools.arkSteedCompileAsync(if_elseif_else_3);

// TODO: Replace the spin loop with:
// let res = ArkTools.waitJitCompileFinish(if_elseif_else_3);
// print(res);
let time = Date.now();
for (let cur = Date.now(); cur - time < 1000; cur = Date.now()) {}

let output = if_elseif_else_3(25, 1, 2, 3);  // globalValue <- 1
print(output);

output = if_elseif_else_3(15, 3, 5, 7);
print(output);

output = if_elseif_else_3(5, 2, 3, 5);  // globalValue <- 3
print(output);
