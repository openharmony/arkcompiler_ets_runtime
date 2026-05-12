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
function switch_simple(x)
{
    let result = 0;

    switch (x) {
        case 1:
            result = 10;
            break;
        case 2:
            result = 20;
            break;
        case 3:
            result = 30;
            break;
        default:
            result = 0;
            break;
    }

    return result;
}

ArkTools.arkSteedCompileAsync(switch_simple);

// TODO: Replace the spin loop with:
// let res = ArkTools.waitJitCompileFinish(switch_simple);
// print(res);
let time = Date.now();
for (let cur = Date.now(); cur - time < 1000; cur = Date.now()) {}

let output = switch_simple(1);
print(output);
output = switch_simple(2);
print(output);
output = switch_simple(3);
print(output);
output = switch_simple(5);
print(output);
