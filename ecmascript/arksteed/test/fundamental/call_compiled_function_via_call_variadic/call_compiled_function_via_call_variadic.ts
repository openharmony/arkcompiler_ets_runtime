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
function call_compiled_function_via_call_variadic_impl(this: { bias: number }, a: number, b: number, c: number): number {
    return this.bias + a + b + c;
}

function call_compiled_function_via_call_variadic() {
    print(call_compiled_function_via_call_variadic_impl.call({ bias: 1 }, 2, 3, 4));
    print(call_compiled_function_via_call_variadic_impl.call({ bias: -5 }, 10, 20, 30));
}

ArkTools.arkSteedCompileAsync(call_compiled_function_via_call_variadic_impl);
ArkTools.arkSteedCompileAsync(call_compiled_function_via_call_variadic);

let time = Date.now();
for (let cur = Date.now(); cur - time < 1000; cur = Date.now()) {}

call_compiled_function_via_call_variadic();
