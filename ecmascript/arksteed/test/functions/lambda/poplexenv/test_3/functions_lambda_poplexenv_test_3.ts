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

// poplexenv in nested lambdas: each lambda defines its own class
let compiled = false;

function call_compiled_lambda_poplexenv_3(x: number, y: number): number {
    const inner = function(b: number): number {
        class InnerMath {
            static factor: number = 3;
            static triple(v: number): number { return v * InnerMath.factor; }
        }
        return InnerMath.triple(b);
    };
    const outer = function(a: number): number {
        class OuterMath {
            static factor: number = 2;
            static double(v: number): number { return v * OuterMath.factor; }
        }
        return OuterMath.double(a) + inner(a);
    };
    if (!compiled) {
        ArkTools.arkSteedCompileSync(outer);
        ArkTools.arkSteedCompileSync(inner);
        compiled = true;
    }
    return outer(x) + y;
}

ArkTools.arkSteedCompileSync(call_compiled_lambda_poplexenv_3);
print(call_compiled_lambda_poplexenv_3(5, 2));
print(call_compiled_lambda_poplexenv_3(3, 10));
print(call_compiled_lambda_poplexenv_3(1, 100));
