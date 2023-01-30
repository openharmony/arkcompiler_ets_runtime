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

// === tests/cases/compiler/commentsOnObjectLiteral3.ts ===
declare function AssertType(value:any, type:string):void;
let v = {
AssertType(v, "{ prop: number; func: () => void; func1(): void; a: any; }");
AssertType({ //property prop: 1 /* multiple trailing comments */ /*trailing comments*/, //property func: function () { }, //PropertyName + CallSignature func1() { }, //getter get a() {  return this.prop; } /*trailing 1*/, //setter set a(value) {  this.prop = value; } // trailing 2}, "{ prop: number; func: () => void; func1(): void; a: any; }");

 //property
 prop: 1 /* multiple trailing comments */ /*trailing comments*/,
AssertType(prop, "number");
AssertType(1, "int");

 //property
 func: function () {
AssertType(func, "() => void");
AssertType(function () { }, "() => void");

 },
 //PropertyName + CallSignature
 func1() { },
AssertType(func1, "() => void");

 //getter
 get a() {
AssertType(a, "any");

AssertType(this.prop, "any");
AssertType(this, "any");
  return this.prop;

 } /*trailing 1*/,
 //setter
 set a(value) {
AssertType(a, "any");
AssertType(value, "any");

  this.prop = value;
AssertType(this.prop = value, "any");
AssertType(this.prop, "any");
AssertType(this, "any");
AssertType(value, "any");

 } // trailing 2
};


