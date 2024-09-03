/*
 * Copyright (c) 2022 Huawei Device Co., Ltd.
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

declare function print(arg: any): string;

class A {
    #a_: number;
    constructor() {
        this.#a_ = 0;
    }
    get #a() {
        return this.#a_;
    }
    set #a(v) {
        this.#a_ = v;
    }
    foo() {
        return this.#a;
    }
    bar() {
        this.#a = 1;
    }
}

for (let i = 0; i < 5; i++) {
    let a = new A();
    print(a.foo());
    a.bar();
    print(a.foo());
}

// hcdata with object (phc)
class B {}
B.prototype.foo = function () {
    return 1;
};

// primitive symbol no pgo
const symbol = Symbol();
const namedSymbol = Symbol("symbol");
print(symbol.toString());
print(namedSymbol.toString());

class BitwiseAnd {
    #setterCalledWith: number = 0;
    get #field() {
        return 0b010111;
    }
    set #field(value) {
        this.#setterCalledWith = value;
    }
    compoundAssignment() {
        return this.#field &= 0b101011;
    }
    setterCalledWithValue() {
        return this.#setterCalledWith;
    }
}

const o = new BitwiseAnd();
// Check that CALL_PRIVATE_GETTER/CALL_PRIVATE_SETTER are processed correctly in NumberSpeculativeRetype
print(o.compoundAssignment());
print(o.setterCalledWithValue());

// Test if the `slotid` is read correctly in the `ldprivateproperty`/`stprivateproperty`
function testReadIcSlotInPrivatePropertyIns() {
    let a;
    class C3 {
        #b;
        constructor() {
            this.#b = Uint8Array;
            this.#b = Uint8Array;
            this.#b = Uint8Array;
            this.#b = Uint8Array;
            this.#b = Uint8Array;
            this.#b = Uint8Array;
            this.#b = Uint8Array;
            this.#b = Uint8Array;
            this.#b = Uint8Array;
            this.#b = Uint8Array;
            this.#b = Uint8Array;
            this.#b = Uint8Array;
            this.#b = Uint8Array;
            this.#b = Uint8Array;
            this.#b = Uint8Array;
            this.#b = Uint8Array;
            this.#b = Uint8Array;
            this.#b = Uint8Array;
            this.#b = Uint8Array;
            this.#b = Uint8Array;
            this.#b = Uint8Array;
            this.#b = Uint8Array;
            this.#b = Uint8Array;
            this.#b = Uint8Array;
            this.#b = Uint8Array;
            this.#b = Uint8Array;
            this.#b = Uint8Array;
            this.#b = Uint8Array;
            this.#b = Uint8Array;
            this.#b = Uint8Array;
            this.#b = Uint8Array;
            this.#b = Uint8Array;
            this.#b = Uint8Array;
            this.#b = Uint8Array;
            this.#b = Uint8Array;
            this.#b = Uint8Array;
            this.#b = Uint8Array;
            this.#b = Uint8Array;
            this.#b = Uint8Array;
            this.#b = Uint8Array;
            this.#b = Uint8Array;
            this.#b = Uint8Array;
            this.#b = Uint8Array;
            this.#b = Uint8Array;
            this.#b = Uint8Array;
            this.#b = Uint8Array;
            this.#b = Uint8Array;
            this.#b = Uint8Array;
            this.#b = Uint8Array;
            this.#b = Uint8Array;
            this.#b = Uint8Array;
            this.#b = Uint8Array;
            this.#b = Uint8Array;
            this.#b = Uint8Array;
            this.#b = Uint8Array;
            this.#b = Uint8Array;
            a = this.#b;
        }
    }

    new C3();
    print("testReadIcSlotInPrivatePropertyIns success")
}

testReadIcSlotInPrivatePropertyIns();

// Invalid private key check for ldPrivateProperty
const v1 = [1];
const v2 = [2];
class C {
    #m(x) {
        x[0] = this;
        return this;
    }
    constructor() {
        try {
            v1.#m(v2);
        } catch(e) {
            print(e.name + " : " + e.message);
        }
    }
}
const v3 = new C();

// Invalid private key check for stPrivateProperty
class OutSide {
    #x = 42;
  
    innerclass() {
      return class {
        f() {
          this.#x = 1;
        }
      }
    }
  }
  
  var Inner = new OutSide().innerclass();
  var i = new Inner();
  
  try {
    i.f();
  } catch(e) {
    print(e.name + " : " + e.message);
}

// Invalid private key check for ldPrivateProperty2
let createClass = function () {
    return class {
      static #m = 111;
  
      static access() {
        return this.#m;
      }
    }
};
  
let C1 = createClass(); 
let C2 = createClass();
try {
    C2.access();
    C1.access.call(C2); // C2 try to access private property of C1, forbidden
} catch(e) {
    print(e)
}

// Invalid private key check for stPrivateProperty2
let createClass2 = function () {
    return class {
      static #m = 111;
  
      static store() {
        return this.#m = 11;
      }
    }
  };
  
  let C3 = createClass2(); 
  let C4 = createClass2();
  
try {
    C3.store();
    C4.store.call(C3); // C3 try to access private property of C4, forbidden
} catch(e) {
    print(e)
}