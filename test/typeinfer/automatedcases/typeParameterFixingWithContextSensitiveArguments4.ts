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

// === tests/cases/compiler/typeParameterFixingWithContextSensitiveArguments4.ts ===
declare function AssertType(value:any, type:string):void;
function f<T, U>(y: T, y1: U, p: (z: U) => T, p1: (x: T) => U): [T, U] { 
AssertType([y, p1(y)], "[T, U]");
return [y, p1(y)]; 

AssertType(y, "T");

AssertType(p1(y), "U");

AssertType(p1, "(T) => U");

AssertType(y, "T");
}

interface A { a: A; }
interface B extends A { b; }

let a: A, b: B;
AssertType(a, "A");
AssertType(b, "B");

let d = f(a, b, x => x, x => <any>x); // Type [A, B]
AssertType(d, "[A, B]");
AssertType(f(a, b, x => x, x => <any>x), "[A, B]");
AssertType(f, "<T, U>(T, U, (U) => T, (T) => U) => [T, U]");
AssertType(a, "A");
AssertType(b, "B");
AssertType(x => x, "(B) => B");
AssertType(x, "B");
AssertType(x, "B");
AssertType(x => <any>x, "(A) => any");
AssertType(x, "A");
AssertType(<any>x, "any");
AssertType(x, "A");


