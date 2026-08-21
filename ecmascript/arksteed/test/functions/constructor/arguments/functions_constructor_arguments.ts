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
class CallCompiledConstructorArgumentsBox {
    x: number = 0;
    y: number = 0;

    constructor() {
        this.x = arguments[0];
        this.y = arguments.length > 1 ? arguments[1] : 5;
    }

    sum(): number {
        return this.x + this.y;
    }
}

function call_compiled_constructor_arguments(): void {
    const a = new CallCompiledConstructorArgumentsBox(2);
    const b = new CallCompiledConstructorArgumentsBox(7, 4);
    print(a.sum());
    print(b.sum());
}

ArkTools.arkSteedCompileSync(CallCompiledConstructorArgumentsBox);
ArkTools.arkSteedCompileSync(CallCompiledConstructorArgumentsBox.prototype.sum);
ArkTools.arkSteedCompileSync(call_compiled_constructor_arguments);


call_compiled_constructor_arguments();
