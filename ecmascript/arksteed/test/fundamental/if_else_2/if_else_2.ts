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
function if_else_2(x, a, b, c)
{
    // if-else
    if (x > 10) {
        a += c;
    } else {
        b -= c;
    }
    return a * 1000 + b;
}

ArkTools.arkSteedCompileSync(if_else_2);

// Spin loop: A temporary approach to wait for ArkSteed JIT compiler

let output = if_else_2(15, 3, 7, 1);
print(output);
output = if_else_2(5, 3, 7, 1);
print(output);
