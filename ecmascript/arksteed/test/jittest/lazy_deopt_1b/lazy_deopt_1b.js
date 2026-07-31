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

// Test Lazy Deopt happened in ldobjbyname.
// After the JIT code for function F is compiled,
// modifying HClass will invalidate F.
// Subsequent accesses to F will detect this invalidation.

function Test(index, obj)
{
    print(`Before call #${index}: isCompiled:`, ArkTools.arkSteedIsCompiled(Test));
    print(obj.x);
    print(`After call #${index}: isCompiled:`, ArkTools.arkSteedIsCompiled(Test));
    print("----------------");
}


class A{}
class B extends A{}
class C extends B{}
A.prototype.x = 1;

let c = new C();
Test(1, c);

ArkTools.arkSteedCompileSync(Test);
Test(2, c);
B.prototype.x = 2;
Test(3, c);

ArkTools.arkSteedCompileSync(Test);
Test(4, c);
C.prototype.x = 3;
Test(5, c);

ArkTools.arkSteedCompileSync(Test);
Test(6, c);
c.x = 4;
Test(7, c);
