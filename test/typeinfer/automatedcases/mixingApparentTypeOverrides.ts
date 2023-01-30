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

// === tests/cases/compiler/mixingApparentTypeOverrides.ts ===
declare function AssertType(value:any, type:string):void;
type Constructor<T> = new(...args: any[]) => T;
function Tagged<T extends Constructor<{}>>(Base: T) {
AssertType(class extends Base {    _tag: string;    constructor(...args: any[]) {      super(...args);      this._tag = "";    }  }, "{ new (...any[]): (Anonymous class); prototype: Tagged<any>.(Anonymous class); } & T");
AssertType(Base, "{}");
  return class extends Base {

    _tag: string;
AssertType(_tag, "string");

    constructor(...args: any[]) {
AssertType(args, "any[]");

      super(...args);
AssertType(super(...args), "void");
AssertType(super, "T");
AssertType(...args, "any");
AssertType(args, "any[]");

      this._tag = "";
AssertType(this._tag = "", "string");
AssertType(this._tag, "string");
AssertType(this, "this");
AssertType("", "string");
    }
  };
}

class A {
  toString () {
AssertType("class A", "string");
    return "class A";
  }
}

class B extends Tagged(A) {
  toString () { // Should not be an error
AssertType("class B", "string");
    return "class B";
  }
}

class C extends A {
  toString () { // Should not be an error
AssertType("class C", "string");
    return "class C";
  }
}

