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
 *    After the JIT code for a function is compiled, modifying its associated HClass
 *    invalidates the function, and subsequent accesses will detect this invalidation.
 * 2. Function Test2 is used to deopt itself by "print("Test2 obj2:", obj2.y)\
 *    and then trigger lazy deoptimization by modifying the prototype,
 * 3. Test2 call ChangePrototypeValue not inlined.
 * 4. Test call Test2 not inlined.
 */

// Function to test lazy deoptimization; triggers a change in the return address of Test2.
// After deoptimization, it checks whether the ASM Interpreter bridge's return address has been replaced.
function ChangePrototypeValue(obj, shouldChange) {
    print("ChangeProto start.");
    if (shouldChange == true) {
        // Modify the 'x' property at the second level of the prototype chain,
        // triggering lazy deoptimization of the JIT-compiled 'test' function.
        obj.__proto__.__proto__.x = 2;
    }
    print("ChangeProto end.");
}

function Test3() {
    print("Test3");
}

// Function Test2 - performs lazy deoptimization and deoptimization.
// It deopt itself by printing a property from obj2 and then calls ChangeProto to trigger lazy deoptimization.
// Additionally, repeated accesses are performed at the end to avoid inlining.
function Test2(obj, obj2, shouldChange) {
    print("Test2 start.");
    
    // Trigger Test2 deopt.
    print("Test2 obj2:", obj2.y);

    // Trigger prototype change and lazy deoptimization.
    ChangePrototypeValue(obj, shouldChange);
    
    Test3();
    print("Test2 end.");
    
    // Additional code to prevent aggressive inlining.
    let test = {};
    test.x; test.x; test.x; test.x; test.x; test.x; test.x; test.x; test.x; test.x;
    test.x; test.x; test.x; test.x; test.x; test.x; test.x; test.x; test.x; test.x;
    test.x; test.x; test.x; test.x; test.x; test.x; test.x; test.x; test.x; test.x;
    test.x; test.x; test.x; test.x; test.x; test.x; test.x; test.x; test.x; test.x;
}

// Function Test calls Test2 to trigger deoptimization and then prints the value of 'x'.
// This function helps verify if the JIT compiled code detected the changes.
function Test(obj, obj2, shouldChange) {
    print("Test start.");
    Test2(obj, obj2, shouldChange);
    print("Test obj.x:", obj.x);
    print("Test end.");
}

class A {}
class B extends A {}
class C extends B {}

// Set initial property values on A's prototype.
A.prototype.x = 1;
A.prototype.y = 1;

let c = new C();
c.y = 2;

// First test: do not modify the prototype, so property 'x' remains unchanged.
Test(c, c, false);

ArkTools.jitCompileAsync(Test2);
print(ArkTools.waitJitCompileFinish(Test2));

ArkTools.jitCompileAsync(Test);
print(ArkTools.waitJitCompileFinish(Test));

print("------------------------------------------------------");
// Second test: change the prototype to trigger lazy deoptimization.
let a = new A();
Test(c, a, true);
