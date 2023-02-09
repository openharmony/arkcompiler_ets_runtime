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

// === tests/cases/conformance/classes/members/privateNames/privateStaticNameShadowing.ts ===
declare function AssertType(value:any, type:string):void;
class X {
    static #f = X.#m();
    constructor() {
      X.#m();
AssertType(X.#m(), "number");
AssertType(X.#m, "() => number");
    }
    static #m() {
      const X: any = {}; // shadow the class
AssertType(X, "any");
AssertType({}, "{}");

      const _a: any = {}; // shadow the first generated let
AssertType(_a, "any");
AssertType({}, "{}");

      X.#m(); // Should check with X as the receiver with _b as the class constructor 
AssertType(X.#m(), "any");
AssertType(X.#m, "any");

AssertType(1, "int");
      return 1;
    }
  }
  

