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

// poplexenv inside compiled function: class defined inside function scope
let compiled = false;

function call_compiled_lambda_with_try_catch_1(x: number): number {
    class LocalMath {
        static factor: number = 2;
        static apply(n: number): number {
            return n * LocalMath.factor;
        }
    }
    const doubler = function(n: number): number {
        return LocalMath.apply(n);
    };
    if (!compiled) {
        ArkTools.arkSteedCompileSync(LocalMath);
        ArkTools.arkSteedCompileSync(LocalMath.apply);
        ArkTools.arkSteedCompileSync(doubler);
        compiled = true;
    }
    return doubler(x);
}

ArkTools.arkSteedCompileSync(call_compiled_lambda_with_try_catch_1);
print(call_compiled_lambda_with_try_catch_1(5));
print(call_compiled_lambda_with_try_catch_1(10));
print(call_compiled_lambda_with_try_catch_1(-3));
