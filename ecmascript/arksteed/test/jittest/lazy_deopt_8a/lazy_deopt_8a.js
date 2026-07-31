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

// Test dependency lazy-deopt while the optimized dependent function is still on stack.
// A reads through the prototype chain, then A -> B -> C -> D -> E -> F.
// F invalidates A's prototype dependency before control returns to A.

class Base {}
class Middle extends Base {}
class Leaf extends Middle {}

Base.prototype.x = 1;

let obj = new Leaf();

function F(shouldChange) {
    if (shouldChange) {
        Middle.prototype.x = 2;
    }
    return "F";
}

function E(shouldChange) {
    return "E" + F(shouldChange);
}

function D(shouldChange) {
    return "D" + E(shouldChange);
}

function C(shouldChange) {
    return "C" + D(shouldChange);
}

function B(shouldChange) {
    return "B" + C(shouldChange);
}

function A(o, shouldChange) {
    let before = o.x;
    let chain = B(shouldChange);
    let after = o.x;
    print("A values:", before, chain, after);
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
