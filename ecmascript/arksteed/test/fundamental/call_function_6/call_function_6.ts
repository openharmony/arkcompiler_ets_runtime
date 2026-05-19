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
function foo(x: number): string {
    return (x ** 3).toString();
}

function call_function_6(x: number, y: number): string {
    return foo(x + y);
}

ArkTools.arkSteedCompileSync(call_function_6);
// TODO: Remove this spin-loop

print(call_function_6(-3, 0.5));
print(call_function_6(5, 10));
