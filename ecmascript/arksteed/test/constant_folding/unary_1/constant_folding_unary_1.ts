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

function constant_folding_unary_1(x: number)
{
    const A = 10;

    let a = -A;      // -10  (NEG)
    let b = ~A;      // -11  (NOT - bitwise not)
    let c = A + 1;   // 11   (ADD2, tests that const propagates)
    let d = A - 1;   // 9    (SUB2)

    return x + a + b + c + d;   // x + (-10) + (-11) + 11 + 9 = x - 1
}

ArkTools.arkSteedCompileSync(constant_folding_unary_1);

print(constant_folding_unary_1(100));
print(constant_folding_unary_1(-100));
