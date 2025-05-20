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
 *    After the JIT code for function 'test' is compiled,
 *    modifying an HClass invalidates the function,
 *    and subsequent accesses to it will detect this invalidation.
 * 2. Test2 call ChangePrototypeValue not inlined.
 * 3. Test call Test2 inlined.
 */


function ChangePrototypeValue(obj, shouldChange) {
    print("ChangeProto start.");
    if (shouldChange) {
        // Update the 'x' property at the second level of the prototype chain,
        // triggering lazy deoptimization of the JIT-compiled 'test' function.
        obj.__proto__.__proto__.x = 2;
    }
    print("ChangeProto end.");
    
    // Additional code to prevent aggressive inlining.
    let test = {};
    test.x; test.x; test.x; test.x; test.x; test.x; test.x; test.x; test.x; test.x;
    test.x; test.x; test.x; test.x; test.x; test.x; test.x; test.x; test.x; test.x;
    test.x; test.x; test.x; test.x; test.x; test.x; test.x; test.x; test.x; test.x;
    test.x; test.x; test.x; test.x; test.x; test.x; test.x; test.x; test.x; test.x;
}

function Test2(obj, shouldChange) {
    print("Test2 start.");
    ChangePrototypeValue(obj, shouldChange);
    ChangePrototypeValue(obj, shouldChange);
    print("Test2 end.");
}

function Test(obj, shouldChange) {
    print("Test start.");
    // Execute inline notification testing.
    Test2(obj, shouldChange);
    // Output the current value of the property 'x'.
    print("Test obj.x :", obj.x);
    print("Test end.");
}

class A {}
class B extends A {}
class C extends B {}

// Set the initial value of 'x' on A's prototype.
A.prototype.x = 1;

const c = new C();

// First test call: without modifying the prototype. Expected output: property 'x' remains unchanged.
Test(c, false);

ArkTools.jitCompileAsync(Test);
print(ArkTools.waitJitCompileFinish(Test));

print("------------------------------------------------------");
// Call test with the flag set to true to modify the prototype property, triggering lazy deoptimization.
Test(c, true);