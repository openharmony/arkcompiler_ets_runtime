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
function switch_true_1(x)
{
    let result = 0;

    switch (true) {
        case x > 0 && x < 3:
            result += 100;
            break;
        case x >= 3 && x < 5:
            result += 200;
            break;
        default:
            result += 0;
            break;
    }

    return result;
}

ArkTools.arkSteedCompileAsync(switch_true_1);

// TODO: Replace the spin loop with:
// let res = ArkTools.waitJitCompileFinish(switch_true_1);
// print(res);
let time = Date.now();
for (let cur = Date.now(); cur - time < 1000; cur = Date.now()) {}

let output = switch_true_1(2);
print(output);
output = switch_true_1(4);
print(output);
output = switch_true_1(-1);
print(output);
output = switch_true_1(5);
print(output);
