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

function constant_folding_edge_bitwise_1(x: number)
{
    const A = 1;

    // 1 << 31 = -2147483648 (SHL max shift boundary)
    let a = A << 31;

    return x + a;
}

ArkTools.arkSteedCompileSync(constant_folding_edge_bitwise_1);

print(constant_folding_edge_bitwise_1(100));
print(constant_folding_edge_bitwise_1(-100));
