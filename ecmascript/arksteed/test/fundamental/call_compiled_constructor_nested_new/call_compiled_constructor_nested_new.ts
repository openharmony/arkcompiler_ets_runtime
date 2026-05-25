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
class CallCompiledConstructorNestedNewInner {
    value: number = 0;

    constructor(x: number) {
        this.value = x + 1;
    }
}

class CallCompiledConstructorNestedNewOuter {
    inner: CallCompiledConstructorNestedNewInner;
    extra: number = 2;

    constructor(base: number) {
        this.inner = new CallCompiledConstructorNestedNewInner(base * 2);
    }

    total(): number {
        return this.inner.value + this.extra;
    }
}

function call_compiled_constructor_nested_new(): void {
    const a = new CallCompiledConstructorNestedNewOuter(3);
    const b = new CallCompiledConstructorNestedNewOuter(1);
    print(a.total());
    print(b.total());
}

ArkTools.arkSteedCompileSync(CallCompiledConstructorNestedNewInner);
ArkTools.arkSteedCompileSync(CallCompiledConstructorNestedNewOuter);
ArkTools.arkSteedCompileSync(CallCompiledConstructorNestedNewOuter.prototype.total);
ArkTools.arkSteedCompileSync(call_compiled_constructor_nested_new);


call_compiled_constructor_nested_new();
