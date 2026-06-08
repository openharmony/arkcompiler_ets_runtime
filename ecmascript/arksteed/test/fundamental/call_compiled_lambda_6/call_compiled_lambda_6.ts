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

// Multiple closures sharing mutable outer scope
let compiled = false;

function call_compiled_lambda_6(a: number, b: number): number {
    let sum: number = 0;
    const addA = function(): void { sum = sum + a; };
    const addB = function(): void { sum = sum + b; };
    if (!compiled) {
        ArkTools.arkSteedCompileSync(addA);
        ArkTools.arkSteedCompileSync(addB);
        compiled = true;
    }
    addA();
    addB();
    addA();
    return sum;
}

ArkTools.arkSteedCompileSync(call_compiled_lambda_6);
print(call_compiled_lambda_6(3, 5));
print(call_compiled_lambda_6(7, 2));
print(call_compiled_lambda_6(1, 10));
