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

Root.prototype.x = 4;
let obj = new Leaf();

function F(shouldChange, value) {
    if (shouldChange) {
        Mid.prototype.x = 9;
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

function A(o, shouldChange, limit) {
    let before = o.x;
    let total = 0;
    for (let i = 0; i < limit; i++) {
        if ((i & 1) === 0) {
            total += before + i;
        } else {
            total += i;
        }
    }
    let chain = B(shouldChange, total);
    let after = o.x;
    for (let j = 0; j < 3; j++) {
        total += after + j;
    }
    print("A loop:", before, chain, after, total, total + chain);
}

print("Before 1st call: isCompiled:", ArkTools.arkSteedIsCompiled(A));
A(obj, false, 5);
print("After 1st call: isCompiled:", ArkTools.arkSteedIsCompiled(A));
print("----------------");

Root.prototype.x = 4;
delete Mid.prototype.x;
ArkTools.arkSteedCompileSync(F);
ArkTools.arkSteedCompileSync(E);
ArkTools.arkSteedCompileSync(D);
ArkTools.arkSteedCompileSync(C);
ArkTools.arkSteedCompileSync(B);
ArkTools.arkSteedCompileSync(A);

print("Before 2nd call: isCompiled:", ArkTools.arkSteedIsCompiled(A));
A(obj, true, 5);
print("After 2nd call: isCompiled:", ArkTools.arkSteedIsCompiled(A));
print("----------------");
