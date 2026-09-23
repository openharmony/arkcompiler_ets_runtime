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
class CallCompiledConstructorDefaultParamBox {
    data: number = 100;

    constructor(x: number = 7, y: number = 9) {
        this.data = x + y;
    }
}

function call_compiled_constructor_default_param(): void {
    const a = new CallCompiledConstructorDefaultParamBox();
    const b = new CallCompiledConstructorDefaultParamBox(3);
    const c = new CallCompiledConstructorDefaultParamBox(3, 4);
    print(a.data);
    print(b.data);
    print(c.data);
}

ArkTools.arkSteedCompileSync(CallCompiledConstructorDefaultParamBox);
ArkTools.arkSteedCompileSync(call_compiled_constructor_default_param);


call_compiled_constructor_default_param();
