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

// === tests/cases/compiler/typeParameterFixingWithContextSensitiveArguments.ts ===
declare function AssertType(value:any, type:string):void;
function f<T, U>(y: T, f: (x: T) => U, x: T): [T, U] { 
AssertType([y, f(x)], "[T, U]");
return [y, f(x)]; 

AssertType(y, "T");

AssertType(f(x), "U");

AssertType(f, "(T) => U");

AssertType(x, "T");
}

interface A { a: A; }
interface B extends A { b; }

let a: A, b: B;
AssertType(a, "A");
AssertType(b, "B");

let d = f(b, x => x.a, a); // type [A, A]
AssertType(d, "[A, A]");
AssertType(f(b, x => x.a, a), "[A, A]");
AssertType(f, "<T, U>(T, (T) => U, T) => [T, U]");
AssertType(b, "B");
AssertType(x => x.a, "(A) => A");
AssertType(x, "A");
AssertType(x.a, "A");
AssertType(a, "A");

let d2 = f(b, x => x.a, null); // type [B, A]
AssertType(d2, "[B, A]");
AssertType(f(b, x => x.a, null), "[B, A]");
AssertType(f, "<T, U>(T, (T) => U, T) => [T, U]");
AssertType(b, "B");
AssertType(x => x.a, "(B) => A");
AssertType(x, "B");
AssertType(x.a, "A");
AssertType(null, "null");

let d3 = f(b, x => x.b, null); // type [B, any]
AssertType(d3, "[B, any]");
AssertType(f(b, x => x.b, null), "[B, any]");
AssertType(f, "<T, U>(T, (T) => U, T) => [T, U]");
AssertType(b, "B");
AssertType(x => x.b, "(B) => any");
AssertType(x, "B");
AssertType(x.b, "any");
AssertType(null, "null");


