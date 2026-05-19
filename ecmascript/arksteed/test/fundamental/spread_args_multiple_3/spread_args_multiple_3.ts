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
function spread_args_multiple_helper(
    x: number, y: number, z: number, u: number, v: number): number {
    return x + y * 10 + z * 100 + u * 1000 + v * 10000;
}

function spread_args_multiple_3(): number {
    const first = [1, 2];
    const second = [4, 5];
    return spread_args_multiple_helper(...first, 3, ...second);
}

ArkTools.arkSteedCompileSync(spread_args_multiple_helper);
ArkTools.arkSteedCompileSync(spread_args_multiple_3);


print(spread_args_multiple_3());
