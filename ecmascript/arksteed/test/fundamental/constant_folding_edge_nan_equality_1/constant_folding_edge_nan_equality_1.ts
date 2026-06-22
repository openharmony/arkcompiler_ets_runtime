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

function constant_folding_edge_nan_equality_1(x: number)
{
    const NAN_VAL: number = NaN;

    let r = 0;
    // NaN !== NaN  is true  (ECMAScript §7.2.16)
    if (NAN_VAL !== NAN_VAL) { r += 1; }   // true  → r=1
    // NaN == NaN   is false (ECMAScript §7.2.15)
    if (NAN_VAL == NAN_VAL)  { r += 10; }  // false → skip
    // null == undefined is true
    if (null == undefined)   { r += 2; }   // true  → r=3
    // null === undefined is false
    if (null === undefined)  { r += 20; }  // false → skip
    // true == 1 is true (Abstract Equality)
    if (true == 1)           { r += 4; }   // true  → r=7

    return x + r;   // x + 7
}

ArkTools.arkSteedCompileSync(constant_folding_edge_nan_equality_1);

print(constant_folding_edge_nan_equality_1(100));
print(constant_folding_edge_nan_equality_1(-100));
