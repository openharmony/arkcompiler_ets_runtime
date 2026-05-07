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
declare function print(arg: number): string;

function foo(x: number): number {
    return x ** 3 + 3 * x + 1;
}

function bar(x: number, y: number): number {
    return foo(x + y) + foo(x - y) + x;
}

function call_compiled_function_4(x: number, y: number): number {
    return foo(x) + foo(y) + bar(x, y) + bar(y, x);
}

ArkTools.arkSteedCompileAsync(foo);
ArkTools.arkSteedCompileAsync(bar);
ArkTools.arkSteedCompileAsync(call_compiled_function_4);

let time = Date.now();
for (let cur = Date.now(); cur - time < 1000; cur = Date.now()) {}

print(call_compiled_function_4(-1.5, 1.75));
print(call_compiled_function_4(2.5, -2.25));
