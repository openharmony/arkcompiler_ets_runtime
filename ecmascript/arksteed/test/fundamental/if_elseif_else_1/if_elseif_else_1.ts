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
function if_elseif_else_1(x)
{
    let result = 0;

    if (x > 20) {
        result = 4;
    } else if (x > 10) {
        result = 5;
    } else {
        result = 6;
    }

    return result;
}

ArkTools.arkSteedCompileSync(if_elseif_else_1);

// TODO: Replace the spin loop with:
// let res = ArkTools.waitJitCompileFinish(if_elseif_else_1);
// print(res);

let output = if_elseif_else_1(5);
print(output);
output = if_elseif_else_1(15);
print(output);
output = if_elseif_else_1(25);
print(output);
