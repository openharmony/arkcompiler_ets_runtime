/*
 * Copyright (c) 2023 Huawei Device Co., Ltd.
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

declare function print(arg:any):string;

function test1() {
    var fromA, fromB;
    class A {}
    class B extends A {}
    class C extends B {
        method() {
            fromA = super['fromA'];
            fromB = super['fromB'];
        }
    }

    A.prototype.fromA = 'a';
    A.prototype.fromB = 'a';
    B.prototype.fromB = 'b';
    C.prototype.fromA = 'c';
    C.prototype.fromB = 'c';

    print(fromA);
    print(fromB);
    C.prototype.method();
    print(fromA);
    print(fromB);
}

test1();

function test2()
{
    function A() {
        this.x = 1;
    }

    function B() {}
    A.prototype = B.prototype;
    let a = new A();
    a.x;
}

test2();
print(ArkTools.isAOTDeoptimized(test2));

class C3 {
  constructor(a5) {
  }
}
class C18 extends C3 {
  constructor() {
    super(1);
    C3.valueOf = 1;
  }
}
const v37 = new C18();
print(C3.valueOf);
print(ArkTools.isTSHClass(C3));
print(ArkTools.isAOTCompiled(C18));

const v23 = Math.constructor;
const v26 = new Proxy([Math,Math], v23);
try {
    v26.shift();
} catch(e28) {
}
function test3() {
    if (!new.target) { throw 'must be called with new'; }
    this[0] = this;
    Math.constructor.freeze(v26);
}
test3.prototype = v26;
const v33 = new test3();
v33[0] = v33;
print(ArkTools.isTSHClass(v33));
