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
class CallCompiledConstructorFieldsBox {
    data: number = 1;
    m: number = 2;
    n: number = 3;

    constructor(x: number) {
        this.data = x;
        if (x % 2 === 0) {
            this.m = 10;
        } else {
            this.n = 20;
        }
    }

    sum(): number {
        return this.data + this.m + this.n;
    }
}

function call_compiled_constructor_fields(): void {
    const a = new CallCompiledConstructorFieldsBox(4);
    const b = new CallCompiledConstructorFieldsBox(5);
    print(a.sum());
    print(b.sum());
}

ArkTools.arkSteedCompileAsync(CallCompiledConstructorFieldsBox);
ArkTools.arkSteedCompileAsync(CallCompiledConstructorFieldsBox.prototype.sum);
ArkTools.arkSteedCompileAsync(call_compiled_constructor_fields);

let time = Date.now();
for (let cur = Date.now(); cur - time < 1000; cur = Date.now()) {}

call_compiled_constructor_fields();
