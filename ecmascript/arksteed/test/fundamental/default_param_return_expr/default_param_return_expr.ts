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
function default_param_return_expr(a: number = 1, b: number = 2, c: number = 3): number {
    let left = a + b;
    let right = b + c;
    return left * right;
}

ArkTools.arkSteedCompileAsync(default_param_return_expr);

let time = Date.now();
for (let cur = Date.now(); cur - time < 1000; cur = Date.now()) {}

print(default_param_return_expr());
print(default_param_return_expr(4));
print(default_param_return_expr(4, 5, 6));
