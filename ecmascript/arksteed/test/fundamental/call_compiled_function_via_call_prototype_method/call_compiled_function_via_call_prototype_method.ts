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
class CallFunctionViaCallPrototypeMethodPoint {
    x: number;
    y: number;

    constructor(x: number, y: number) {
        this.x = x;
        this.y = y;
    }

    sum(): number {
        return this.x + this.y;
    }
}

function call_compiled_function_via_call_prototype_method() {
    let point = new CallFunctionViaCallPrototypeMethodPoint(4, 6);
    print(CallFunctionViaCallPrototypeMethodPoint.prototype.sum.call(point));
    print(CallFunctionViaCallPrototypeMethodPoint.prototype.sum.call({ x: 10, y: 20 }));
}

ArkTools.arkSteedCompileAsync(CallFunctionViaCallPrototypeMethodPoint.prototype.sum);
ArkTools.arkSteedCompileAsync(call_compiled_function_via_call_prototype_method);

let time = Date.now();
for (let cur = Date.now(); cur - time < 1000; cur = Date.now()) {}

call_compiled_function_via_call_prototype_method();
