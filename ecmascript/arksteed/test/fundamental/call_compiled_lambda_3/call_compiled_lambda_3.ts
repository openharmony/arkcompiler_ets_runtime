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
let compiled = false;

function call_compiled_lambda_3(x: number): number {
    const foo = (a: number, b: number) => a * b;
    if (!compiled) {
        ArkTools.arkSteedCompileAsync(foo);
        // Do not remove this spin-loop. This is already part of the test case
        let time = Date.now();
        for (let cur = Date.now(); cur - time < 1000; cur = Date.now()) {}
        compiled = true;
    }
    return foo(x - 1, x + 2);
}

(async () => {
    await ArkTools.arkSteedCompileAsync(call_compiled_lambda_3);
    // TODO: Remove this spin-loop
    let time = Date.now();
    for (let cur = Date.now(); cur - time < 1000; cur = Date.now()) {}
})().then(() => {
    print(call_compiled_lambda_3(-2.5));
    print(call_compiled_lambda_3(15));
});
