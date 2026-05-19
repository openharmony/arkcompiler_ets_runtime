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
function spread_args_basic_helper(x: number, y: number, z: number): number {
    return x + y + z;
}

function spread_args_basic_2(): number {
    const arr: [number, number, number] = [10, 20, 30];
    return spread_args_basic_helper(...arr);
}

ArkTools.arkSteedCompileSync(spread_args_basic_helper);
ArkTools.arkSteedCompileSync(spread_args_basic_2);


print(spread_args_basic_2());
