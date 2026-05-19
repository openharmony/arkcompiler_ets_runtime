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
class CallCompiledConstructorDefaultParamLengthBox {
    len: number;
    value: number;

    constructor(x: number = 7) {
        this.len = arguments.length;
        this.value = x;
    }
}

function call_compiled_constructor_default_param_length_2(
    x?: number): CallCompiledConstructorDefaultParamLengthBox {
    return x === undefined ?
        new CallCompiledConstructorDefaultParamLengthBox() :
        new CallCompiledConstructorDefaultParamLengthBox(x);
}

ArkTools.arkSteedCompileSync(CallCompiledConstructorDefaultParamLengthBox);
ArkTools.arkSteedCompileSync(call_compiled_constructor_default_param_length_2);


let a = call_compiled_constructor_default_param_length_2();
print(a.len);
print(a.value);

let b = call_compiled_constructor_default_param_length_2(4);
print(b.len);
print(b.value);
