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

// === tests/cases/compiler/superPropertyAccess_ES6.ts ===
declare function AssertType(value:any, type:string):void;
class MyBase {
  getValue(): number { 
AssertType(1, "int");
return 1; 
}

  get value(): number { 
AssertType(1, "int");
return 1; 
}
}

class MyDerived extends MyBase {
  constructor() {
    super();
AssertType(super(), "void");
AssertType(super, "typeof MyBase");

    const f1 = super.getValue();
AssertType(f1, "number");
AssertType(super.getValue(), "number");
AssertType(super.getValue, "() => number");
AssertType(super, "MyBase");

    const f2 = super.value;
AssertType(f2, "number");
AssertType(super.value, "number");
AssertType(super, "MyBase");
  }
}

let d = new MyDerived();
AssertType(d, "MyDerived");
AssertType(new MyDerived(), "MyDerived");
AssertType(MyDerived, "typeof MyDerived");

let f3 = d.value;
AssertType(f3, "number");
AssertType(d.value, "number");

class A {
    private _property: string;
    get property() { 
AssertType(this._property, "string");
return this._property; 

AssertType(this, "this");
}

    set property(value: string) { this._property = value 
AssertType(this._property = value, "string");

AssertType(this._property, "string");

AssertType(this, "this");

AssertType(value, "string");
}
}

class B extends A {
    set property(value: string) {
        super.property = value + " addition";
AssertType(super.property = value + " addition", "string");
AssertType(super.property, "string");
AssertType(super, "A");
AssertType(value + " addition", "string");
AssertType(value, "string");
AssertType(" addition", "string");
    }
}

