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
function call_compiled_function_via_call_method_borrow_impl(this: { scale: number }, value: number): number {
    return this.scale * value;
}

let call_compiled_function_via_call_method_borrow_source = {
    scale: 3,
    mul: call_compiled_function_via_call_method_borrow_impl
};

function call_compiled_function_via_call_method_borrow() {
    let target = { scale: 5 };
    print(call_compiled_function_via_call_method_borrow_source.mul.call(target, 4));
    print(call_compiled_function_via_call_method_borrow_source.mul.call({ scale: 2 }, 9));
}

ArkTools.arkSteedCompileAsync(call_compiled_function_via_call_method_borrow_impl);
ArkTools.arkSteedCompileAsync(call_compiled_function_via_call_method_borrow);

let time = Date.now();
for (let cur = Date.now(); cur - time < 1000; cur = Date.now()) {}

call_compiled_function_via_call_method_borrow();
