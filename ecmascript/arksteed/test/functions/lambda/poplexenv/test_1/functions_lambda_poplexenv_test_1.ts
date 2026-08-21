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

// poplexenv in compiled lambda: class defined inside the lambda
let compiled = false;

function call_compiled_lambda_poplexenv_1(x: number): number {
    const calc = function(n: number): number {
        class Doubler {
            static factor: number = 2;
            static scale(v: number): number { return v * Doubler.factor; }
        }
        return Doubler.scale(n);
    };
    if (!compiled) {
        ArkTools.arkSteedCompileSync(calc);
        compiled = true;
    }
    return calc(x);
}

ArkTools.arkSteedCompileSync(call_compiled_lambda_poplexenv_1);
print(call_compiled_lambda_poplexenv_1(5));
print(call_compiled_lambda_poplexenv_1(0));
print(call_compiled_lambda_poplexenv_1(-7));
