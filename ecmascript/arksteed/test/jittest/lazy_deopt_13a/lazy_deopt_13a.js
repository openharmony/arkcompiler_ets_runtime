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


class Root {}
class Mid extends Root {}
class Leaf extends Mid {}

Root.prototype.x = 6;
let obj = new Leaf();

function F(shouldChange, value) {
    if (shouldChange) {
        Mid.prototype.x = 13;
    }
    return value + 1;
}

function E(shouldChange, value) {
    return F(shouldChange, value) + 2;
}

function D(shouldChange, value) {
    return E(shouldChange, value) + 3;
}

function C(shouldChange, value) {
    return D(shouldChange, value) + 4;
}

function B(shouldChange, value) {
    return C(shouldChange, value) + 5;
}

function A(o, shouldChange, p, q) {
    let before = o.x;
    let base = before;
    if (p > q) {
        base += p * 2;
        if ((base & 1) === 0) {
            base += 7;
        } else {
            base -= 1;
        }
    } else {
        base += q;
    }
    let carry = 1;
    for (let i = 0; i < 4; i++) {
        carry += (base + i) % 5;
    }
    let chain = B(shouldChange, base + carry);
    let after = o.x;
    let post = chain;
    if (after > before) {
        for (let k = 0; k < 3; k++) {
            post += after - k;
        }
    } else {
        post += before;
    }
    print("A complex:", before, base, carry, chain, after, post);
}

print("Before 1st call: isCompiled:", ArkTools.arkSteedIsCompiled(A));
A(obj, false, 5, 2);
print("After 1st call: isCompiled:", ArkTools.arkSteedIsCompiled(A));
print("----------------");

Root.prototype.x = 6;
delete Mid.prototype.x;
ArkTools.arkSteedCompileSync(F);
ArkTools.arkSteedCompileSync(E);
ArkTools.arkSteedCompileSync(D);
ArkTools.arkSteedCompileSync(C);
ArkTools.arkSteedCompileSync(B);
ArkTools.arkSteedCompileSync(A);

print("Before 2nd call: isCompiled:", ArkTools.arkSteedIsCompiled(A));
A(obj, true, 5, 2);
print("After 2nd call: isCompiled:", ArkTools.arkSteedIsCompiled(A));
print("----------------");
