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
function loop_for_1(limit, p, q)
{
    let sum = 0;

    for (let i = 0; i < limit; i++) {
        sum += i * p + q;
    }

    return sum;
}

ArkTools.arkSteedCompileSync(loop_for_1);

// TODO: Replace the spin loop with:
// let res = ArkTools.waitJitCompileFinish(loop_for_1);
// print(res);

let output = loop_for_1(0, 3, 1);
print(output);
output = loop_for_1(5, 7, 2);
print(output);
output = loop_for_1(10, 11, 3);
print(output);
