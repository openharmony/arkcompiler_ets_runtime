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

// poplexenv in recursive lambda: class inside recursive lambda
let compiled = false;

function call_compiled_lambda_poplexenv_4(n: number): number {
    const factorial = function(x: number): number {
        class Helper {
            static zeroVal: number = 0;
            static isZero(v: number): boolean { return v == Helper.zeroVal; }
        }
        if (Helper.isZero(x)) {
            return 1;
        }
        return x * factorial(x - 1);
    };
    if (!compiled) {
        ArkTools.arkSteedCompileSync(factorial);
        compiled = true;
    }
    return factorial(n);
}

ArkTools.arkSteedCompileSync(call_compiled_lambda_poplexenv_4);
print(call_compiled_lambda_poplexenv_4(5));
print(call_compiled_lambda_poplexenv_4(0));
print(call_compiled_lambda_poplexenv_4(3));
