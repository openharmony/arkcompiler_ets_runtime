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

function Test(index, obj)
{
    print(`Before call #${index}: isCompiled:`, ArkTools.arkSteedIsCompiled(Test));
    print("Test value:", obj.x);
    print(`After call #${index}: isCompiled:`, ArkTools.arkSteedIsCompiled(Test));
    print("----------------");
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
Test(1, g);

ArkTools.arkSteedCompileSync(Test);
Test(2, g);
C.prototype.x = 20;
Test(3, g);

ArkTools.arkSteedCompileSync(Test);
Test(4, g);
E.prototype.x = 30;
Test(5, g);

ArkTools.arkSteedCompileSync(Test);
Test(6, g);
g.x = 40;
Test(7, g);
