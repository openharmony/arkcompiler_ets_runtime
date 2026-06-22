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

function constant_folding_bitwise_1(x: number)
{
    const A: number = 0xFF;   // 255
    const B: number = 0x0F;   // 15

    let a = A & B;   // 255 & 15 = 15
    let b = A | B;   // 255 | 15 = 255
    let c = a ^ b;   // 15 ^ 255 = 240
    let d = A << 2;  // 255 << 2 = 1020
    let e = d >> 2;  // 1020 >> 2 = 255
    let f = c ^ e;   // 240 ^ 255 = 15
    let g = A ^ B;   // 255 ^ 15 = 240

    return x + a + b + c + d + e + f + g;
    // x + 15 + 255 + 240 + 1020 + 255 + 15 + 240
    // = x + 2040
}

ArkTools.arkSteedCompileSync(constant_folding_bitwise_1);

print(constant_folding_bitwise_1(100));
print(constant_folding_bitwise_1(-100));
