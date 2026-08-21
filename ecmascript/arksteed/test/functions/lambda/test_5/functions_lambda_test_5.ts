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

// Closure: lambda captures and mutates outer variable
let compiled = false;

function call_compiled_lambda_5(init: number, x: number): number {
    let acc: number = init;
    const add = function(n: number): number {
        acc = acc + n;
        return acc;
    };
    if (!compiled) {
        ArkTools.arkSteedCompileSync(add);
        compiled = true;
    }
    return add(x);
}

ArkTools.arkSteedCompileSync(call_compiled_lambda_5);
print(call_compiled_lambda_5(0, 5));
print(call_compiled_lambda_5(10, 7));
print(call_compiled_lambda_5(3, -2));
