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

// === tests/cases/compiler/overloadOnGenericClassAndNonGenericClass.ts ===
declare function AssertType(value:any, type:string):void;
class A { a; }
class B { b; }
class C { c; }
class X<T> { x: T; }
class X1 { x: string; }
class X2 { x: string; }
function f(a: X1): A;
function f<T>(a: X<T>): B;
function f(a): any {
}

let xs: X<string>;
AssertType(xs, "X<string>");

let t3 = f(xs);
AssertType(t3, "A");
AssertType(f(xs), "A");
AssertType(f, "{ (X1): A; <T>(X<T>): B; }");
AssertType(xs, "X<string>");

let t3: A; // should not error
AssertType(t3, "A");


