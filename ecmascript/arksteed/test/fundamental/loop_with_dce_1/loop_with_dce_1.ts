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

let counter = 0;

function foo()
{
    return (counter++) * 100;
}

function loop_with_dce_1(n: number)
{
    let result = 0;
    for (let i = 0; i < n; i++) {
        if (true) {
            result += i;
        } else {
            result += foo();
        }
    }
    return result;
}

ArkTools.arkSteedCompileSync(loop_with_dce_1);

print(loop_with_dce_1(10));
print(counter);
