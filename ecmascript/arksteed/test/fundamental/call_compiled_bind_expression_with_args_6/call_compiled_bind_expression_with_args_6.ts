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
class BoundClosureTarget {
    values: number[]

    constructor(...values: number[]) {
        this.values = values;
        return this.run.bind(this, ...values);
    }

    run(x: number, y: number, z: number, a: number, b: number, c: number): number {
        return a * x + b * y + c * z;
    }
}

function call_compiled_bind_expression_with_args_6(run: (...args: number[]) => number,
                                                   ...args: number[]): number {
    return run(...args);
}

ArkTools.arkSteedCompileAsync(BoundClosureTarget.prototype.run);
ArkTools.arkSteedCompileAsync(call_compiled_bind_expression_with_args_6);

let time = Date.now();
for (let cur = Date.now(); cur - time < 1000; cur = Date.now()) {}

print(call_compiled_bind_expression_with_args_6(new BoundClosureTarget(), 1000, 100, 10, 4, 5, 6));
print(call_compiled_bind_expression_with_args_6(new BoundClosureTarget(1000), 100, 10, 4, 5, 6));
print(call_compiled_bind_expression_with_args_6(new BoundClosureTarget(1000, 100), 10, 4, 5, 6));
print(call_compiled_bind_expression_with_args_6(new BoundClosureTarget(1000, 100, 10), 4, 5, 6));
print(call_compiled_bind_expression_with_args_6(new BoundClosureTarget(1000, 100, 10, 4), 5, 6));
print(call_compiled_bind_expression_with_args_6(new BoundClosureTarget(1000, 100, 10, 4, 5), 6));
print(call_compiled_bind_expression_with_args_6(new BoundClosureTarget(1000, 100, 10, 4, 5, 6)));
