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

// poplexenv in lambda + closure: class method captures outer variable
let compiled = false;

function call_compiled_lambda_poplexenv_2(base: number, x: number): number {
    const transform = function(n: number): number {
        class Scaler {
            static factor: number = 1;
            static apply(v: number): number { return v * base + Scaler.factor; }
        }
        return Scaler.apply(n);
    };
    if (!compiled) {
        ArkTools.arkSteedCompileSync(transform);
        compiled = true;
    }
    return transform(x);
}

ArkTools.arkSteedCompileSync(call_compiled_lambda_poplexenv_2);
print(call_compiled_lambda_poplexenv_2(3, 5));
print(call_compiled_lambda_poplexenv_2(10, 7));
print(call_compiled_lambda_poplexenv_2(2, -3));
