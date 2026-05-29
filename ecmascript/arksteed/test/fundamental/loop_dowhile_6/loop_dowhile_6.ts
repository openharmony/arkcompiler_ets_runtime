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

function loop_dowhile_6(n: number, p: number)
{
    let res: number;
    do {
        res = n;
        n *= 2;
        p -= 1;
    } while (p > 0);
    return res;
}

ArkTools.arkSteedCompileAsync(loop_dowhile_6);

let time = Date.now();
for (let cur = Date.now(); cur - time < 1000; cur = Date.now()) {}

let output = loop_dowhile_6(10, 0);
print(output);
output = loop_dowhile_6(10, 1);
print(output);
output = loop_dowhile_6(10, 2);
print(output);
output = loop_dowhile_6(10, 3);
print(output);
