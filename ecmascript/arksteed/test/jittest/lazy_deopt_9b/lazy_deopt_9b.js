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

// Same live-local chain test as lazy_deopt_9a, but observes A's state inside A.

class Root {}
class Mid extends Root {}
class Leaf extends Mid {}

Root.prototype.x = 10;
let obj = new Leaf();

function F(delta, shouldChange) {
    if (shouldChange) {
        Mid.prototype.x = 20;
    }
    return delta + 6;
}

function E(delta, shouldChange) {
    return F(delta + 5, shouldChange) + 50;
}

function D(delta, shouldChange) {
    return E(delta + 4, shouldChange) + 40;
}

function C(delta, shouldChange) {
    return D(delta + 3, shouldChange) + 30;
}

function B(delta, shouldChange) {
    return C(delta + 2, shouldChange) + 20;
}

function A(index, o, seed, shouldChange) {
    print(`Before call #${index}: isCompiled:`, ArkTools.arkSteedIsCompiled(A));
    let before = o.x;
    let left = seed + before;
    let right = left * 2;
    let chain = B(right, shouldChange);
    let after = o.x;
    print("A result:", before, left, right, chain, after, left + right + chain + after);
    print(`After call #${index}: isCompiled:`, ArkTools.arkSteedIsCompiled(A));
    print("----------------");
}

A(1, obj, 3, false);

ArkTools.arkSteedCompileSync(F);
ArkTools.arkSteedCompileSync(E);
ArkTools.arkSteedCompileSync(D);
ArkTools.arkSteedCompileSync(C);
ArkTools.arkSteedCompileSync(B);
ArkTools.arkSteedCompileSync(A);
A(2, obj, 3, true);
