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

// Based on lazy_deopt_14, but covers both dependency lazy-deopt and numeric eager-deopt.

function Test(index, obj, bias)
{
    print(`Before call #${index}: isCompiled:`, ArkTools.arkSteedIsCompiled(Test));
    let value = obj.x;
    let result = value - bias;
    print("Test value:", value, bias, result);
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
Test(1, g, 1);

ArkTools.arkSteedCompileSync(Test);
Test(2, g, 1);
C.prototype.x = 20;
Test(3, g, 2);

ArkTools.arkSteedCompileSync(Test);
Test(4, g, 2);
Test(5, g, "eager");

ArkTools.arkSteedCompileSync(Test);
Test(6, g, 3);
E.prototype.x = 30;
Test(7, g, 3);
