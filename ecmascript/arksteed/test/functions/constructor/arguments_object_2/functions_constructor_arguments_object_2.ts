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
class CallCompiledConstructorArgumentsObjectBox {
    first: number;
    second: number;

    constructor(x?: number, y?: number) {
        this.first = arguments[0] === undefined ? -1 : arguments[0];
        this.second = arguments[1] === undefined ? -1 : arguments[1];
    }
}

function call_compiled_constructor_arguments_object_2(
    x?: number, y?: number): CallCompiledConstructorArgumentsObjectBox {
    return new CallCompiledConstructorArgumentsObjectBox(x, y);
}

ArkTools.arkSteedCompileSync(CallCompiledConstructorArgumentsObjectBox);
ArkTools.arkSteedCompileSync(call_compiled_constructor_arguments_object_2);


let a = call_compiled_constructor_arguments_object_2();
print(a.first);
print(a.second);

let b = call_compiled_constructor_arguments_object_2(4);
print(b.first);
print(b.second);

let c = call_compiled_constructor_arguments_object_2(4, 5);
print(c.first);
print(c.second);
