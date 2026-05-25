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
function call_function_4(x: number): number {
    if (x <= 0.5) {
        return x + 0.1;
    }
    return 10 * call_function_4(x - 1) + call_function_4(x / 2);
}

ArkTools.arkSteedCompileSync(call_function_4);
// TODO: Remove this spin-loop

print(call_function_4(0));
print(call_function_4(1));
print(call_function_4(2));
print(call_function_4(3));
print(call_function_4(4));
print(call_function_4(5));
print(call_function_4(6));
print(call_function_4(7));
print(call_function_4(8));
print(call_function_4(9));
