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

/*
 * Description:
 * 1. This code tests the lazy deoptimization that occurs in ldobjbyname.
 *    After the JIT code for function 'Test2' is compiled,
 *    modifying an HClass invalidates the function,
 *    and subsequent accesses to it will detect this invalidation.
 * 2. Speciall Test3 throw a exception before lazy deoptimization.
 *    It verifies that exceptions are handled correctly when lazy deoptimization is bypassed.
 *    Note: In this situation, lazy deoptimization will not be triggered.
 */

function Test3(shouldChange)
{
    if (shouldChange) {
        ArkTools.forceLazyDeopt(C.prototype, 1, true);
    }
    
    // Additional code to prevent aggressive inlining.
    let test = {};
    test.x; test.x; test.x; test.x; test.x; test.x; test.x; test.x; test.x; test.x;
    test.x; test.x; test.x; test.x; test.x; test.x; test.x; test.x; test.x; test.x;
    test.x; test.x; test.x; test.x; test.x; test.x; test.x; test.x; test.x; test.x;
    test.x; test.x; test.x; test.x; test.x; test.x; test.x; test.x; test.x; test.x;
}

function Test2(obj, shouldChange) {
    obj.x;
    Test3(shouldChange);
}

class A {}
class B extends A {}
class C extends B {}

// Set the initial value of property x through A.prototype.
A.prototype.x = 1;

const c = new C();

// Initial call to test without changing the prototype's property.
Test2(c, false);

ArkTools.jitCompileAsync(Test2);
print(ArkTools.waitJitCompileFinish(Test2));

print("------------------------------------------------------");
try {
    Test2(c, true);
} catch (e) {
    print(e);
}