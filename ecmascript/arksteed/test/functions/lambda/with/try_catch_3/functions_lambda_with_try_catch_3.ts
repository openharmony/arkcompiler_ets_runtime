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

// Multi-level poplexenv: class in function + class in nested lambda
let compiled = false;

function call_compiled_lambda_with_try_catch_3(base: number, mode: number): number {
    class OuterCalc {
        static offset: number = 5;
        static adjust(x: number): number {
            return x + OuterCalc.offset;
        }
    }
    const factory = function(inc: number) {
        class InnerCalc {
            static scale: number = 2;
            static transform(v: number): number {
                return v * InnerCalc.scale + inc;
            }
        }
        return function(x: number): number {
            return InnerCalc.transform(OuterCalc.adjust(x));
        };
    };
    if (!compiled) {
        ArkTools.arkSteedCompileSync(OuterCalc);
        ArkTools.arkSteedCompileSync(OuterCalc.adjust);
        ArkTools.arkSteedCompileSync(factory);
        compiled = true;
    }
    const fn = factory(base);
    return fn(mode);
}

ArkTools.arkSteedCompileSync(call_compiled_lambda_with_try_catch_3);
print(call_compiled_lambda_with_try_catch_3(3, 1));
print(call_compiled_lambda_with_try_catch_3(0, 10));
print(call_compiled_lambda_with_try_catch_3(7, 2));
