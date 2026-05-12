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
function switch_true_fallthrough(x, a, b, c, d)
{
    let count = 0;
    switch (true) {
        case x > 0:
            count = count + 1;
        case x >= 0:
            count = count + 10;
            if (x === 0) {
                break;
            }
        case x !== 0:
            count = count + 100;
            return count + a;
        default:
            count = count + 1000;
            return count + d;
    }
    return count + b + c;
}

ArkTools.arkSteedCompileAsync(switch_true_fallthrough);

let time = Date.now();
for (let cur = Date.now(); cur - time < 1000; cur = Date.now()) {}

print(switch_true_fallthrough(5, 10, 20, 30, 40));
print(switch_true_fallthrough(0, 10, 20, 30, 40));
print(switch_true_fallthrough(-5, 10, 20, 30, 40));

print(switch_true_fallthrough(5, 1, 20, 300, 4000));
print(switch_true_fallthrough(0, 1, 20, 300, 4000));
print(switch_true_fallthrough(-5, 1, 20, 300, 4000));
