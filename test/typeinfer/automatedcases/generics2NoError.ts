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

// === tests/cases/compiler/generics2NoError.ts ===
declare function AssertType(value:any, type:string):void;
interface A { a: string; }
interface B extends A { b: string; }
interface C extends B { c: string; }
interface G<T, U extends B> {
    x: T;
    y: U;
}


let v1: {
AssertType(v1, "{ x: {    a: string;}; y: {    a: string;    b: string;    c: string;}; }");

    x: { a: string; 
AssertType(x, "{ a: string; }");

AssertType(a, "string");
}

    y: { a: string; b: string; c: string };
AssertType(y, "{ a: string; b: string; c: string; }");
AssertType(a, "string");
AssertType(b, "string");
AssertType(c, "string");

}; // Ok


let v2: G<{ a: string }, C>;   // Ok, equivalent to G<A, C>
AssertType(v2, "G<{ a: string; }, C>");
AssertType(a, "string");

let v4: G<G<A, B>, C>;         // Ok
AssertType(v4, "G<G<A, B>, C>");


