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
class ConstructorCallInitThrowBox {
    data: number = 1;

    constructor(x?: number) {
        if (x === undefined) {
            throw new Error("x is required");
        }
        this.data = x;
    }
}

function call_compiled_constructor_exception_no_throw(): void {
    const a = new ConstructorCallInitThrowBox(1);
    print(a.data);
    const b = new ConstructorCallInitThrowBox(5);
    print(b.data);
}

ArkTools.arkSteedCompileAsync(ConstructorCallInitThrowBox);
ArkTools.arkSteedCompileAsync(call_compiled_constructor_exception_no_throw);

let time = Date.now();
for (let cur = Date.now(); cur - time < 1000; cur = Date.now()) {}

call_compiled_constructor_exception_no_throw();
