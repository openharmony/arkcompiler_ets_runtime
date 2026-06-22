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

function constant_folding_comparison_1(x: number)
{
    const A = 10;
    const B = 20;

    let r = 0;
    if (A < B)   { r += 1; }   // true  → r=1
    if (A > B)   { r += 10; }  // false → skipped
    if (A <= B)  { r += 2; }   // true  → r=3
    if (A >= B)  { r += 20; }  // false → skipped
    if (A == 10) { r += 4; }   // true  → r=7
    if (A != 10) { r += 40; }  // false → skipped
    if (A === B) { r += 100; } // false → skipped

    return x + r;   // x + 7
}

ArkTools.arkSteedCompileSync(constant_folding_comparison_1);

print(constant_folding_comparison_1(100));
print(constant_folding_comparison_1(-100));
