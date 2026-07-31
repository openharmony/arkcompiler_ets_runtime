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

Root.prototype.x = 5;
let obj = new Leaf();

function F(shouldChange, value) {
    if (shouldChange) {
        Mid.prototype.x = 11;
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

function A(o, shouldChange, mode) {
    let before = o.x;
    let score = 0;
    if (mode > 0) {
        score += before + 3;
    } else {
        score += before - 3;
    }
    let marker = "cold";
    if (mode === 2) {
        marker = "branch-T";
    } else {
        marker = "branch-F";
    }
    let chain = B(shouldChange, score);
    let after = o.x;
    if (after > 10) {
        score += after * 2;
    } else {
        score += after;
    }
    print("A branch:", before, marker, chain, after, score + chain);
}

print("Before 1st call: isCompiled:", ArkTools.arkSteedIsCompiled(A));
A(obj, false, 2);
print("After 1st call: isCompiled:", ArkTools.arkSteedIsCompiled(A));
print("----------------");

Root.prototype.x = 5;
delete Mid.prototype.x;
ArkTools.arkSteedCompileSync(F);
ArkTools.arkSteedCompileSync(E);
ArkTools.arkSteedCompileSync(D);
ArkTools.arkSteedCompileSync(C);
ArkTools.arkSteedCompileSync(B);
ArkTools.arkSteedCompileSync(A);

print("Before 2nd call: isCompiled:", ArkTools.arkSteedIsCompiled(A));
A(obj, true, 2);
print("After 2nd call: isCompiled:", ArkTools.arkSteedIsCompiled(A));
print("----------------");
