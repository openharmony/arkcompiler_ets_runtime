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

// === tests/cases/compiler/index.js ===
declare function AssertType(value:any, type:string):void;
export { }
let obj = {
AssertType(obj, "{ x: number; y: number[]; fun: typeof fun; f2: () => void; f3: typeof (Anonymous function); }");
AssertType({  x: 10,  y: [1],  fun: function() {    this.x = 1    this/*1*/  },  f2: function() {    this.x    this/*2*/  },  f3: (function() {    this.x = 1    this/*3*/  }),}, "{ x: number; y: number[]; fun: typeof fun; f2: () => void; f3: typeof (Anonymous function); }");

  x: 10,
AssertType(x, "number");
AssertType(10, "int");

  y: [1],
AssertType(y, "number[]");
AssertType([1], "number[]");
AssertType(1, "int");

  fun: function() {
AssertType(fun, "typeof fun");
AssertType(function() {    this.x = 1    this/*1*/  }, "typeof fun");

    this.x = 1
AssertType(this.x = 1, "int");
AssertType(this.x, "number");
AssertType(this, "{ x: number; y: number[]; fun: typeof fun; f2: () => void; f3: typeof (Anonymous function); }");
AssertType(1, "int");

    this/*1*/
AssertType(this, "{ x: number; y: number[]; fun: typeof fun; f2: () => void; f3: typeof (Anonymous function); }");

  },
  f2: function() {
AssertType(f2, "() => void");
AssertType(function() {    this.x    this/*2*/  }, "() => void");

    this.x
AssertType(this.x, "number");
AssertType(this, "{ x: number; y: number[]; fun: typeof fun; f2: () => void; f3: typeof (Anonymous function); }");

    this/*2*/
AssertType(this, "{ x: number; y: number[]; fun: typeof fun; f2: () => void; f3: typeof (Anonymous function); }");

  },
  f3: (function() {
AssertType(f3, "typeof (Anonymous function)");
AssertType((function() {    this.x = 1    this/*3*/  }), "typeof (Anonymous function)");
AssertType(function() {    this.x = 1    this/*3*/  }, "typeof (Anonymous function)");

    this.x = 1
AssertType(this.x = 1, "int");
AssertType(this.x, "any");
AssertType(this, "any");
AssertType(1, "int");

    this/*3*/
AssertType(this, "any");

  }),
}


