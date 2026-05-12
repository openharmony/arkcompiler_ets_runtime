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
function loop_return_do_while(n, target, add)
{
    let sum = 0;
    let i = 0;
    do {
        sum += (i + add);
        if (i == target) {
            return i + add;
        }
        sum += (i + add) * 4;
        i++;
    } while (i < n);
    return sum;
}

ArkTools.arkSteedCompileAsync(loop_return_do_while);

let time = Date.now();
for (let cur = Date.now(); cur - time < 1000; cur = Date.now()) {}

print(loop_return_do_while(15, 5, 10));
print(loop_return_do_while(8, 3, 7));
print(loop_return_do_while(12, 8, 4));
