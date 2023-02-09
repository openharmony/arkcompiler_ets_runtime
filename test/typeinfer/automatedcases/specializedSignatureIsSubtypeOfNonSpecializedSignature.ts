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

// === tests/cases/conformance/types/objectTypeLiteral/callSignatures/specializedSignatureIsSubtypeOfNonSpecializedSignature.ts ===
declare function AssertType(value:any, type:string):void;
// Specialized signatures must be a subtype of a non-specialized signature
// All the below should not be errors

function foo(x: 'a');
function foo(x: string);
function foo(x: any) { }

class C {
    foo(x: 'a');
    foo(x: string);
    foo(x: any) { }
}

class C2<T> {
    foo(x: 'a');
    foo(x: string);
    foo(x: T);
    foo(x: any) { }
}

class C3<T extends String> {
    foo(x: 'a');
    foo(x: string);
    foo(x: T);
    foo(x: any) { }
}

interface I {
    (x: 'a');
    (x: number);
    (x: string);
    foo(x: 'a');
    foo(x: string);
    foo(x: number);
}

interface I2<T> {
    (x: 'a');
    (x: T);
    (x: string);
    foo(x: 'a');
    foo(x: string);
    foo(x: T);
}

interface I3<T extends String> {
    (x: 'a');
    (x: string);
    (x: T);
    foo(x: 'a');
    foo(x: string);
    foo(x: T);
}

let a: {
AssertType(a, "{ (string): any; ('a'): any; (number): any; foo(string): any; foo('a'): any; foo(number): any; }");

    (x: string);
AssertType(x, "string");

    (x: 'a');
AssertType(x, "string");

    (x: number);
AssertType(x, "number");

    foo(x: string);
AssertType(foo, "{ (string): any; ("a"): any; (number): any; }");
AssertType(x, "string");

    foo(x: 'a');
AssertType(foo, "{ (string): any; ('a'): any; (number): any; }");
AssertType(x, "string");

    foo(x: number);
AssertType(foo, "{ (string): any; ("a"): any; (number): any; }");
AssertType(x, "number");
}

let a2: {
AssertType(a2, "{ ('a'): any; (string): any; <T>(T): any; foo(string): any; foo('a'): any; foo<T>(T): any; }");

    (x: 'a');
AssertType(x, "string");

    (x: string);
AssertType(x, "string");

    <T>(x: T);
AssertType(x, "T");

    foo(x: string);
AssertType(foo, "{ (string): any; ("a"): any; <T>(T): any; }");
AssertType(x, "string");

    foo(x: 'a');
AssertType(foo, "{ (string): any; ('a'): any; <T>(T): any; }");
AssertType(x, "string");

    foo<T>(x: T);
AssertType(foo, "{ (string): any; ("a"): any; <T>(T): any; }");
AssertType(x, "T");
}

let a3: {
AssertType(a3, "{ ('a'): any; <T>(T): any; (string): any; foo(string): any; foo('a'): any; foo<T extends String>(T): any; }");

    (x: 'a');
AssertType(x, "string");

    <T>(x: T);
AssertType(x, "T");

    (x: string);
AssertType(x, "string");

    foo(x: string);
AssertType(foo, "{ (string): any; ("a"): any; <T extends String>(T): any; }");
AssertType(x, "string");

    foo(x: 'a');
AssertType(foo, "{ (string): any; ('a'): any; <T extends String>(T): any; }");
AssertType(x, "string");

    foo<T extends String>(x: T);
AssertType(foo, "{ (string): any; ("a"): any; <T extends String>(T): any; }");
AssertType(x, "T");
}


