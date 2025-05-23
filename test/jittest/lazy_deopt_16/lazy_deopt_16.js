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
 *    After the JIT code for function 'f' is compiled,
 *    modifying an HClass invalidates the function,
 *    and subsequent accesses to it will detect this invalidation.
 * 2. Lazy deopt occurs on constructor.
 */

class A {}
let a2 = new A();
A.prototype.__proto__.x = 1;
let cnt = 0;
function g()
{
    cnt++;
    if (cnt == 2) {
        // Change the property 'x' at the the prototype chain,
        // triggering lazy deoptimization of the JIT-compiled 'f' function.
        A.prototype.x = 2;
    }
}
class B extends A{
    constructor() {
        super();
        g();
        this.name = 233;
        return 233;
    }
}

function f() {
    try {
        let a = new B();
        print(a.name);
    } catch (e) {
        print("Exception:", e);
    }
    print(a2.x);
}

f();
ArkTools.jitCompileAsync(f);
print(ArkTools.waitJitCompileFinish(f));

print("------------------------------------------------------");
f();