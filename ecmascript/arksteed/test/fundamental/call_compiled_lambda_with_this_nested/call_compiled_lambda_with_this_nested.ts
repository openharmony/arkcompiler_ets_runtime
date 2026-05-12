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
let call_compiled_lambda_with_this_nested_outer_compiled = false;
let call_compiled_lambda_with_this_nested_inner_compiled = false;

class CallCompiledLambdaWithThisNestedBox {
    base: number;

    constructor(base: number) {
        this.base = base;
    }

    buildNested(factor: number): () => number {
        const outer = () => {
            const inner = () => this.base * factor;
            if (!call_compiled_lambda_with_this_nested_inner_compiled) {
                ArkTools.arkSteedCompileAsync(inner);
                let time = Date.now();
                for (let cur = Date.now(); cur - time < 1000; cur = Date.now()) {}
                call_compiled_lambda_with_this_nested_inner_compiled = true;
            }
            return inner();
        };
        if (!call_compiled_lambda_with_this_nested_outer_compiled) {
            ArkTools.arkSteedCompileAsync(outer);
            let time = Date.now();
            for (let cur = Date.now(); cur - time < 1000; cur = Date.now()) {}
            call_compiled_lambda_with_this_nested_outer_compiled = true;
        }
        return outer;
    }
}

function call_compiled_lambda_with_this_nested(): void {
    const box1 = new CallCompiledLambdaWithThisNestedBox(6);
    const box2 = new CallCompiledLambdaWithThisNestedBox(7);
    print(box1.buildNested(4)());
    print(box2.buildNested(5)());
}

ArkTools.arkSteedCompileAsync(call_compiled_lambda_with_this_nested);
ArkTools.arkSteedCompileAsync(CallCompiledLambdaWithThisNestedBox.prototype.buildNested);
let time = Date.now();
for (let cur = Date.now(); cur - time < 1000; cur = Date.now()) {}

call_compiled_lambda_with_this_nested();
