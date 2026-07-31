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

// Based on lazy_deopt_1, but stretches the prototype chain.

function Test(obj)
{
    print("Test value:", obj.x);
}

class A {}
class B extends A {}
class C extends B {}
class D extends C {}
class E extends D {}
class F extends E {}
class G extends F {}

A.prototype.x = 10;

let g = new G();
print("Before 1st call: isCompiled:", ArkTools.arkSteedIsCompiled(Test));
Test(g);
print("After 1st call: isCompiled:", ArkTools.arkSteedIsCompiled(Test));
print("----------------");

ArkTools.arkSteedCompileSync(Test);
print("Before 2nd call: isCompiled:", ArkTools.arkSteedIsCompiled(Test));
Test(g);
print("After 2nd call: isCompiled:", ArkTools.arkSteedIsCompiled(Test));
print("----------------");

C.prototype.x = 20;
print("Before 3rd call: isCompiled:", ArkTools.arkSteedIsCompiled(Test));
Test(g);
print("After 3rd call: isCompiled:", ArkTools.arkSteedIsCompiled(Test));
print("----------------");

ArkTools.arkSteedCompileSync(Test);
print("Before 4th call: isCompiled:", ArkTools.arkSteedIsCompiled(Test));
Test(g);
print("After 4th call: isCompiled:", ArkTools.arkSteedIsCompiled(Test));
print("----------------");

E.prototype.x = 30;
print("Before 5th call: isCompiled:", ArkTools.arkSteedIsCompiled(Test));
Test(g);
print("After 5th call: isCompiled:", ArkTools.arkSteedIsCompiled(Test));
print("----------------");

ArkTools.arkSteedCompileSync(Test);
print("Before 6th call: isCompiled:", ArkTools.arkSteedIsCompiled(Test));
Test(g);
print("After 6th call: isCompiled:", ArkTools.arkSteedIsCompiled(Test));
print("----------------");

g.x = 40;
print("Before 7th call: isCompiled:", ArkTools.arkSteedIsCompiled(Test));
Test(g);
print("After 7th call: isCompiled:", ArkTools.arkSteedIsCompiled(Test));
print("----------------");
