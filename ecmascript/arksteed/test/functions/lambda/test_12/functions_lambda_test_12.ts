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

// Multi-scope closure factory: inner lambda captures from 2 outer scopes
let compiled = false;

function call_compiled_lambda_12(w: number, x: number, y: number, z: number): number {
    let baseVal: number = w;
    const factory = function(inc: number) {
        let scale: number = 2;
        return function(v: number): number {
            return (baseVal + inc + v) * scale;
        };
    };
    const f1 = factory(x);
    const f2 = factory(y);
    if (!compiled) {
        ArkTools.arkSteedCompileSync(factory);
        ArkTools.arkSteedCompileSync(f1);
        ArkTools.arkSteedCompileSync(f2);
        compiled = true;
    }
    return f1(z) + f2(z);
}

ArkTools.arkSteedCompileSync(call_compiled_lambda_12);
print(call_compiled_lambda_12(1, 2, 3, 4));
print(call_compiled_lambda_12(5, 1, 1, 10));
print(call_compiled_lambda_12(0, 5, 10, 3));
