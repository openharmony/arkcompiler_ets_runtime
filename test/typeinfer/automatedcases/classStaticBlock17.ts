/*
* Copyright (c) Microsoft Corporation. All rights reserved.
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
*
* This file has been modified by Huawei to verify type inference by adding verification statements.
*/

// === tests/cases/conformance/classes/classStaticBlock/classStaticBlock17.ts ===
declare function AssertType(value:any, type:string):void;
let friendA: { getX(o: A): number, setX(o: A, v: number): void };
AssertType(friendA, "{ getX(A): number; setX(A, number): void; }");
AssertType(getX, "(A) => number");
AssertType(o, "A");
AssertType(setX, "(A, number) => void");
AssertType(o, "A");
AssertType(v, "number");

class A {
  #x: number;

  constructor (v: number) {
    this.#x = v;
AssertType(this.#x = v, "number");
AssertType(this.#x, "number");
AssertType(this, "this");
AssertType(v, "number");
  }

  getX () {
AssertType(this.#x, "number");
AssertType(this, "this");
    return this.#x;
  }

  static {
    friendA = {
AssertType(friendA, "{ getX(A): number; setX(A, number): void; }");
AssertType({      getX(obj) { return obj.#x },      setX(obj, value) { obj.#x = value }    }, "{ getX(A): number; setX(A, number): void; }");
AssertType(friendA = {      getX(obj) { return obj.#x },      setX(obj, value) { obj.#x = value }    }, "{ getX(A): number; setX(A, number): void; }");

      getX(obj) { 
AssertType(getX, "(A) => number");
AssertType(obj, "A");
AssertType(obj.#x, "number");
return obj.#x },

      setX(obj, value) { obj.#x = value 
AssertType(setX, "(A, number) => void");

AssertType(obj, "A");

AssertType(value, "number");

AssertType(obj.#x = value, "number");

AssertType(obj.#x, "number");

AssertType(value, "number");
}

    };
  }
};

class B {
  constructor(a: A) {
    const x = friendA.getX(a); // ok
AssertType(x, "number");
AssertType(friendA.getX(a), "number");
AssertType(friendA.getX, "(A) => number");
AssertType(a, "A");

    friendA.setX(a, x + 1); // ok
AssertType(friendA.setX(a, x + 1), "void");
AssertType(friendA.setX, "(A, number) => void");
AssertType(a, "A");
AssertType(x + 1, "number");
AssertType(x, "number");
AssertType(1, "int");
  }
};

const a = new A(41);
AssertType(a, "A");
AssertType(new A(41), "A");
AssertType(A, "typeof A");
AssertType(41, "int");

const b = new B(a);
AssertType(b, "B");
AssertType(new B(a), "B");
AssertType(B, "typeof B");
AssertType(a, "A");

a.getX();
AssertType(a.getX(), "number");
AssertType(a.getX, "() => number");


