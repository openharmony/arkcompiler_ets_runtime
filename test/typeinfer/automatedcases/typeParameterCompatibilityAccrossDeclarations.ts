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

// === tests/cases/compiler/typeParameterCompatibilityAccrossDeclarations.ts ===
declare function AssertType(value:any, type:string):void;
let a = {
AssertType(a, "{ x: <T>(T) => T; }");
AssertType({   x: function <T>(y: T): T { return null; }}, "{ x: <T>(T) => T; }");

   x: function <T>(y: T): T { 
AssertType(x, "<T>(T) => T");
return null; 

AssertType(function <T>(y: T): T { return null; }, "<T>(T) => T");

AssertType(y, "T");

AssertType(null, "null");
}
}
let a2 = {
AssertType(a2, "{ x: (any) => any; }");
AssertType({   x: function (y: any): any { return null; }}, "{ x: (any) => any; }");

   x: function (y: any): any { 
AssertType(x, "(any) => any");
return null; 

AssertType(function (y: any): any { return null; }, "(any) => any");

AssertType(y, "any");

AssertType(null, "null");
}
}
export interface I {
   x<T>(y: T): T;
}
export interface I2 {
   x(y: any): any;
}
 
let i: I;
AssertType(i, "I");

let i2: I2;
AssertType(i2, "I2");
 
a = i; // error
AssertType(a = i, "I");
AssertType(a, "{ x: <T>(T) => T; }");
AssertType(i, "I");

i = a; // error
AssertType(i = a, "{ x: <T>(T) => T; }");
AssertType(i, "I");
AssertType(a, "{ x: <T>(T) => T; }");
 
a2 = i2; // no error
AssertType(a2 = i2, "I2");
AssertType(a2, "{ x: (any) => any; }");
AssertType(i2, "I2");

i2 = a2; // no error
AssertType(i2 = a2, "{ x: (any) => any; }");
AssertType(i2, "I2");
AssertType(a2, "{ x: (any) => any; }");


