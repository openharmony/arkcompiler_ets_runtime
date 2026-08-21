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

// Currying: lambda that returns a lambda with captured outer context
let compiled = false;

function call_compiled_lambda_7(x: number, y: number, z: number): number {
    const makeAdder = function(offset: number) {
        return function(val: number): number {
            return offset + val;
        };
    };
    const addX = makeAdder(x);
    const addY = makeAdder(y);
    if (!compiled) {
        ArkTools.arkSteedCompileSync(makeAdder);
        ArkTools.arkSteedCompileSync(addX);
        ArkTools.arkSteedCompileSync(addY);
        compiled = true;
    }
    return addX(z) + addY(z);
}

ArkTools.arkSteedCompileSync(call_compiled_lambda_7);
print(call_compiled_lambda_7(5, 3, 2));
print(call_compiled_lambda_7(10, 20, 5));
print(call_compiled_lambda_7(0, 100, 50));
