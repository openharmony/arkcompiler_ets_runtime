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
class BaseClass {
    baseValue: number;

    constructor(baseValue: number) {
        this.baseValue = baseValue;
    }

    compute(a: number, b: number): number {
        return this.baseValue + a + b;
    }
}

class DerivedClass extends BaseClass {
    multiplier: number;

    constructor(baseValue: number, multiplier: number) {
        super(baseValue);
        this.multiplier = multiplier;
    }

    multiply(baseVal: number, x: number): number {
        return this.compute(baseVal, x) * this.multiplier;
    }
}

function call_compiled_bind_expression_with_args_4(run: (x: number) => number): number {
    return run(3);
}

ArkTools.arkSteedCompileAsync(BaseClass.prototype.compute);
ArkTools.arkSteedCompileAsync(DerivedClass.prototype.multiply);
ArkTools.arkSteedCompileAsync(call_compiled_bind_expression_with_args_4);

let time = Date.now();
for (let cur = Date.now(); cur - time < 1000; cur = Date.now()) {}

let obj = new DerivedClass(10, 5);
let boundFn = obj.multiply.bind(obj, 10);
print(call_compiled_bind_expression_with_args_4(boundFn));