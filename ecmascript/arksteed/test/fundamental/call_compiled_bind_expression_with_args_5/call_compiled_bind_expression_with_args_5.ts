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
class MultiBoundTarget {
    fn1: (a: number, b: number) => number;
    fn2: (a: number, b: number) => number;

    constructor(x: number, y: number) {
        this.fn1 = this.add.bind(this, x);
        this.fn2 = this.multiply.bind(this, y);
    }

    add(base: number, n: number): number {
        return base + n;
    }

    multiply(base: number, n: number): number {
        return base * n;
    }
}

function call_compiled_bind_expression_with_args_5(
    fn1: (base: number, n: number) => number,
    fn2: (base: number, n: number) => number,
    n: number
): number {
    return fn1(10, n) + fn2(10, n);
}

ArkTools.arkSteedCompileAsync(MultiBoundTarget.prototype.add);
ArkTools.arkSteedCompileAsync(MultiBoundTarget.prototype.multiply);
ArkTools.arkSteedCompileAsync(call_compiled_bind_expression_with_args_5);

let time = Date.now();
for (let cur = Date.now(); cur - time < 1000; cur = Date.now()) {}

let target = new MultiBoundTarget(5, 3);
print(call_compiled_bind_expression_with_args_5(target.fn1, target.fn2, 4));