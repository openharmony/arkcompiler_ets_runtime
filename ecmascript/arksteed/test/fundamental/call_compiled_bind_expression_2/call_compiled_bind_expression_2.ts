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
function square(x: number): number { return x * x; }

class BoundClosureTarget {
    value: number;

    constructor(value: number) {
        this.value = value;
        return this.run.bind(this);
    }

    run(n: number): number {
        return square(n);
    }
}

function call_compiled_bind_expression_2(run: (n: number) => number, n: number): number {
    return run(n);
}

ArkTools.arkSteedCompileAsync(BoundClosureTarget.prototype.run);
ArkTools.arkSteedCompileAsync(call_compiled_bind_expression_2);

let time = Date.now();
for (let cur = Date.now(); cur - time < 1000; cur = Date.now()) {}

print(call_compiled_bind_expression_2(new BoundClosureTarget(42), 10));
