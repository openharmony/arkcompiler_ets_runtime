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
class CallCompiledConstructorInheritanceBase {
    base: number = 1;

    constructor(x: number) {
        this.base = x + 1;
    }
}

class CallCompiledConstructorInheritanceDerived extends CallCompiledConstructorInheritanceBase {
    delta: number = 2;

    constructor(x: number, y: number) {
        super(x);
        this.delta = y * 2;
    }

    value(): number {
        return this.base + this.delta;
    }
}

function call_compiled_constructor_inheritance(): void {
    const a = new CallCompiledConstructorInheritanceDerived(2, 3);
    const b = new CallCompiledConstructorInheritanceDerived(0, 4);
    print(a.value());
    print(b.value());
}

ArkTools.arkSteedCompileSync(CallCompiledConstructorInheritanceBase);
ArkTools.arkSteedCompileSync(CallCompiledConstructorInheritanceDerived);
ArkTools.arkSteedCompileSync(CallCompiledConstructorInheritanceDerived.prototype.value);
ArkTools.arkSteedCompileSync(call_compiled_constructor_inheritance);


call_compiled_constructor_inheritance();
