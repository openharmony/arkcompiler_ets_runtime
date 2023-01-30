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

// === tests/cases/compiler/typeParameterFixingWithContextSensitiveArguments5.ts ===
declare function AssertType(value:any, type:string):void;
function f<T, U>(t1: T, u1: U, pf1: (u2: U) => T, pf2: (t2: T) => U): [T, U] { 
AssertType([t1, pf2(t1)], "[T, U]");
return [t1, pf2(t1)]; 

AssertType(t1, "T");

AssertType(pf2(t1), "U");

AssertType(pf2, "(T) => U");

AssertType(t1, "T");
}

interface A { a: A; }
interface B extends A { b: any; }

let a: A, b: B;
AssertType(a, "A");
AssertType(b, "B");

let d = f(a, b, u2 => u2.b, t2 => t2);
AssertType(d, "[any, B]");
AssertType(f(a, b, u2 => u2.b, t2 => t2), "[any, B]");
AssertType(f, "<T, U>(T, U, (U) => T, (T) => U) => [T, U]");
AssertType(a, "A");
AssertType(b, "B");
AssertType(u2 => u2.b, "(B) => any");
AssertType(u2, "B");
AssertType(u2.b, "any");
AssertType(t2 => t2, "(any) => any");
AssertType(t2, "any");
AssertType(t2, "any");


