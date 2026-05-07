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
let call_compiled_lambda_with_this_call_rebind_compiled = false;

class CallCompiledLambdaWithThisCallRebindBox {
    value: number;

    constructor(value: number) {
        this.value = value;
    }

    buildReader(): (offset: number) => number {
        const foo = (offset: number) => this.value + offset;
        if (!call_compiled_lambda_with_this_call_rebind_compiled) {
            ArkTools.arkSteedCompileAsync(foo);
            let time = Date.now();
            for (let cur = Date.now(); cur - time < 1000; cur = Date.now()) {}
            call_compiled_lambda_with_this_call_rebind_compiled = true;
        }
        return foo;
    }
}

function call_compiled_lambda_with_this_call_rebind(): void {
    const box = new CallCompiledLambdaWithThisCallRebindBox(20);
    const reader = box.buildReader();
    print(reader(2));
    print(reader.call({ value: 100 }, 3));
}

ArkTools.arkSteedCompileAsync(call_compiled_lambda_with_this_call_rebind);
ArkTools.arkSteedCompileAsync(CallCompiledLambdaWithThisCallRebindBox.prototype.buildReader);
let time = Date.now();
for (let cur = Date.now(); cur - time < 1000; cur = Date.now()) {}

call_compiled_lambda_with_this_call_rebind();
