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

// F invalidates A after branch and loop work in the deep callee.
// This stresses lazy-deopt through a non-trivial JIT call chain.

class Root {}
class Mid extends Root {}
class Leaf extends Mid {}

Root.prototype.x = 7;
let obj = new Leaf();

function F(flag, n) {
    let acc = 0;
    for (let i = 0; i < n; i++) {
        if ((i & 1) === 0) {
            acc += i + 1;
        } else {
            acc += i * 2;
        }
    }
    if (flag) {
        Mid.prototype.x = acc;
    }
    return acc;
}

function E(flag, n) {
    return F(flag, n) + 5;
}

function D(flag, n) {
    return E(flag, n + 1) + 4;
}

function C(flag, n) {
    return D(flag, n + 1) + 3;
}

function B(flag, n) {
    return C(flag, n + 1) + 2;
}

function A(o, flag) {
    let before = o.x;
    let chain = B(flag, 2);
    let after = o.x;
    print("A deep:", before, chain, after, before + chain + after);
}

print("Before 1st call: isCompiled:", ArkTools.arkSteedIsCompiled(A));
A(obj, false);
print("After 1st call: isCompiled:", ArkTools.arkSteedIsCompiled(A));
print("----------------");

ArkTools.arkSteedCompileSync(F);
ArkTools.arkSteedCompileSync(E);
ArkTools.arkSteedCompileSync(D);
ArkTools.arkSteedCompileSync(C);
ArkTools.arkSteedCompileSync(B);
ArkTools.arkSteedCompileSync(A);

print("Before 2nd call: isCompiled:", ArkTools.arkSteedIsCompiled(A));
A(obj, true);
print("After 2nd call: isCompiled:", ArkTools.arkSteedIsCompiled(A));
print("----------------");
