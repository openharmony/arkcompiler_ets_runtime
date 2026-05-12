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
function spread_args_nested_helper(
    a: number, b: number, c: number, d: number, e: number, f: number): number {
    return a + 10 * b + 100 * c + 1000 * d + 10000 * e + 100000 * f;
}

function spread_args_nested_1(): number {
    const inner: [number, number] = [3, 4];
    const outer: [number, number, number, number] = [1, 2, ...inner];
    return spread_args_nested_helper(5, ...outer, 6);
}

ArkTools.arkSteedCompileAsync(spread_args_nested_helper);
ArkTools.arkSteedCompileAsync(spread_args_nested_1);

let time = Date.now();
for (let cur = Date.now(); cur - time < 1000; cur = Date.now()) {}

print(spread_args_nested_1());
