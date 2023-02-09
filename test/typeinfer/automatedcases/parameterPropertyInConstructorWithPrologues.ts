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

// === tests/cases/compiler/parameterPropertyInConstructorWithPrologues.ts ===
declare function AssertType(value:any, type:string):void;
// https://github.com/microsoft/TypeScript/issues/48671

class C {}

class Foo1 {
  constructor(private A: string) {
    "ngInject1";
AssertType("ngInject1", "string");
  }
}

class Foo2 {
  constructor(private A: string, private B: string) {
    "ngInject1";
AssertType("ngInject1", "string");

    "ngInject2";
AssertType("ngInject2", "string");
  }
}

class Foo3 {
  constructor(private A: string, private B: string, private C: string) {
    "ngInject1";
AssertType("ngInject1", "string");

    "ngInject2";
AssertType("ngInject2", "string");
  }
}

class Foo4 {
  constructor(private A: string) {
    "ngInject1";
AssertType("ngInject1", "string");

    console.log("hi");
AssertType(console.log("hi"), "void");
AssertType(console.log, "(...any[]) => void");
AssertType("hi", "string");
  }
}

class Foo5 {
  constructor(private A: string, private B: string) {
    "ngInject1";
AssertType("ngInject1", "string");

    "ngInject2";
AssertType("ngInject2", "string");

    console.log("hi");
AssertType(console.log("hi"), "void");
AssertType(console.log, "(...any[]) => void");
AssertType("hi", "string");
  }
}

class Foo6 {
  constructor(private A: string, private B: string, private C: string) {
    "ngInject1";
AssertType("ngInject1", "string");

    "ngInject2";
AssertType("ngInject2", "string");

    console.log("hi");
AssertType(console.log("hi"), "void");
AssertType(console.log, "(...any[]) => void");
AssertType("hi", "string");
  }
}

class Foo7 extends C {
  constructor(
    private member: boolean,
  ) {
    "ngInject1";
AssertType("ngInject1", "string");

    super();
AssertType(super(), "void");
AssertType(super, "typeof C");

    console.log("hi");
AssertType(console.log("hi"), "void");
AssertType(console.log, "(...any[]) => void");
AssertType("hi", "string");
  }
}

class Foo8 extends C {
  constructor(
    private member: boolean,
  ) {
    "ngInject1";
AssertType("ngInject1", "string");

    super();
AssertType(super(), "void");
AssertType(super, "typeof C");

    this.m();
AssertType(this.m(), "void");
AssertType(this.m, "() => void");
AssertType(this, "this");

    console.log("hi");
AssertType(console.log("hi"), "void");
AssertType(console.log, "(...any[]) => void");
AssertType("hi", "string");
  }

  m() {}
}

class Foo9 extends C {
  constructor() {
    "ngInject1";
AssertType("ngInject1", "string");

    "ngInject2";
AssertType("ngInject2", "string");

    super();
AssertType(super(), "void");
AssertType(super, "typeof C");

    this.m();
AssertType(this.m(), "void");
AssertType(this.m, "() => void");
AssertType(this, "this");

    console.log("hi");
AssertType(console.log("hi"), "void");
AssertType(console.log, "(...any[]) => void");
AssertType("hi", "string");
  }

  m() {}
}


