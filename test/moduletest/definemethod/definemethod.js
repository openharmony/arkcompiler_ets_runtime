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

// static_initializer
class A {
	static a = 1;
}
print(A.a);

// instance_initializer
class B {
    b = "b"
}
let b = new B()
print(b.b);

// getter, setter, static
class C {
    get getv() {
        return this.elt;
    }

    set setv(num) {
        this.elt = num;
    }

    static sgetv() {
        return this.elt;
    }

    static ssetv(num) {
        this.elt = num;
    }

    elt = 1;
}
let c = new C;
c.setv = 0x5aa5;
print(c.getv);
C.ssetv(0xa55a);
print(C.sgetv());

// #~D^1>#
var empty = Object.create(null);
empty.x = "c";
var D, value;
for (D = class { get ['x' in empty]() {return 'via get';}};;) {
    value = D.prototype.false;
    break;
}

for (D = class { set ['x' in empty](param) {value = param;}};;) {
    D.prototype.false = 'via set';
    break;
}
print(D.prototype.false);

// #~E>@1~@2>#instance_initializer
class E {
    #outer = 'test';
  
    B_withoutPrivateField = class {
      method(o) {
        return o.#outer;
      }
    }
  
    B_withPrivateField = class {
      #inner = 42;
      method(o) {
        return this.#inner;
      }
    }
}
let e = new(E);
let e1 = new(e.B_withoutPrivateField);
let e2 = new(e.B_withPrivateField);
print(e1.method(e));
print(e2.method(e2));
