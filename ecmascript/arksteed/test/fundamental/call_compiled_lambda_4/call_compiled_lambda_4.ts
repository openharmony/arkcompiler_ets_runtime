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

function call_compiled_lambda_4(x: number, y: number): number {
    const foo = (a: number, b: number): number => ((b == 0) ? a : foo(b, a % b));
    if (!compiled) {
        ArkTools.arkSteedCompileSync(foo);
        // Do not remove this spin-loop. This is already part of the test case
        compiled = true;
    }
    return foo(x, y);
}

(async () => {
    await ArkTools.arkSteedCompileSync(call_compiled_lambda_4);
    // TODO: Remove this spin-loop
})().then(() => {
    print(call_compiled_lambda_4(24, 36));
    print(call_compiled_lambda_4(15, 48));
    print(call_compiled_lambda_4(20, 12));
});
