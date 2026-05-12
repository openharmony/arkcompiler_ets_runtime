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
function loop_return_conditional(n, target, fallback, offset)
{
    let sum = 0;
    for (let i = 0; i < n; i++) {
        sum += (i + offset);
        if (i == target) {
            return i * offset;
        }
        sum += (i + offset) * 2;
        if (i == fallback) {
            return i + offset * 10;
        }
        sum += (i + offset) * 3;
    }
    return sum;
}

ArkTools.arkSteedCompileAsync(loop_return_conditional);

let time = Date.now();
for (let cur = Date.now(); cur - time < 1000; cur = Date.now()) {}

print(loop_return_conditional(15, 3, 7, 2));
print(loop_return_conditional(12, 7, 3, 5));
print(loop_return_conditional(20, 5, 12, 3));
