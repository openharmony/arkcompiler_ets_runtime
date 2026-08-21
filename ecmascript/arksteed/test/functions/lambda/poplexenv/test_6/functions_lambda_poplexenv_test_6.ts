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

// poplexenv in factory + inner lambdas: each level has its own class
let compiled = false;

function call_compiled_lambda_poplexenv_6(factor: number, x: number, y: number): number {
    const factory = function(f: number) {
        class Multiplier {
            static offset: number = 0;
            static scale(v: number): number { return v * f + Multiplier.offset; }
        }
        return function(v: number): number {
            return Multiplier.scale(v);
        };
    };
    const f1 = factory(factor);
    const f2 = factory(factor * 2);
    if (!compiled) {
        ArkTools.arkSteedCompileSync(factory);
        ArkTools.arkSteedCompileSync(f1);
        ArkTools.arkSteedCompileSync(f2);
        compiled = true;
    }
    return f1(x) + f2(y);
}

ArkTools.arkSteedCompileSync(call_compiled_lambda_poplexenv_6);
print(call_compiled_lambda_poplexenv_6(3, 5, 2));
print(call_compiled_lambda_poplexenv_6(2, 10, 7));
print(call_compiled_lambda_poplexenv_6(1, 3, 8));
