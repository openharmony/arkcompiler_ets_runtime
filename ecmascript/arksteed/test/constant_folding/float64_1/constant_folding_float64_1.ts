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

function constant_folding_float64_1(x: number)
{
    const A = 2.5;
    const B = 3.5;

    let y = A + B;   // 6.0  (ADD2)
    let z = A * B;   // 8.75 (MUL2)
    y = y - 1.5;     // 4.5  (SUB2)
    z = z / 2.5;     // 3.5  (DIV2)

    return x + y + z;   // x + 8.0
}

ArkTools.arkSteedCompileSync(constant_folding_float64_1);

print(constant_folding_float64_1(100));
print(constant_folding_float64_1(-100));
