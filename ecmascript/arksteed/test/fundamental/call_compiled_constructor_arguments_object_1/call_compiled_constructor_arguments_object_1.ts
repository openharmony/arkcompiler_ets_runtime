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

function call_compiled_constructor_arguments_object_1(): void {
    let a = new CallCompiledConstructorArgumentsObjectBox();
    print(a.first);
    print(a.second);

    let b = new CallCompiledConstructorArgumentsObjectBox(4);
    print(b.first);
    print(b.second);

    let c = new CallCompiledConstructorArgumentsObjectBox(4, 5);
    print(c.first);
    print(c.second);
}

ArkTools.arkSteedCompileAsync(CallCompiledConstructorArgumentsObjectBox);
ArkTools.arkSteedCompileAsync(call_compiled_constructor_arguments_object_1);

let time = Date.now();
for (let cur = Date.now(); cur - time < 1000; cur = Date.now()) {}

call_compiled_constructor_arguments_object_1();
