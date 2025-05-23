/*
 * Copyright (c) 2025 Huawei Device Co., Ltd.
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

function Test(obj)
{
    print(obj.x);
}

class A{}
class B extends A{}
class C extends B{}
A.prototype.x = 1;

let c = new C();
Test(c);

ArkTools.jitCompileAsync(Test);
print(ArkTools.waitJitCompileFinish(Test));

Test(c);

B.prototype.x = 2;
Test(c);

ArkTools.jitCompileAsync(Test);
print(ArkTools.waitJitCompileFinish(Test));

Test(c);

C.prototype.x = 3;
Test(c);

ArkTools.jitCompileAsync(Test);
print(ArkTools.waitJitCompileFinish(Test));

Test(c);

c.x = 4;
Test(c);