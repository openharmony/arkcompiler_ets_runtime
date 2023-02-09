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

// === tests/cases/conformance/expressions/contextualTyping/functionExpressionContextualTyping1.ts ===
declare function AssertType(value:any, type:string):void;
// When a function expression with no type parameters and no parameter type annotations 
// is contextually typed (section 4.19) by a type T and a contextual signature S can be extracted from T

enum E { red, blue }

// A contextual signature S is extracted from a function type T as follows:
//      If T is a function type with exactly one call signature, and if that call signature is non- generic, S is that signature.

let a0: (n: number, s: string) => number = (num, str) => {
AssertType(a0, "(number, string) => number");
AssertType(n, "number");
AssertType(s, "string");
AssertType(num, "number");
AssertType(str, "string");
AssertType((num, str) => {    num.toExponential();    return 0;}, "(number, string) => number");

    num.toExponential();
AssertType(num.toExponential(), "string");
AssertType(num.toExponential, "(?number) => string");

AssertType(0, "int");
    return 0;
}

class Class<T> {
    foo() { }
}

let a1: (c: Class<Number>) => number = (a1) => {
AssertType(a1, "(Class<Number>) => number");
AssertType(c, "Class<Number>");
AssertType(a1, "Class<Number>");
AssertType((a1) => {    a1.foo();    return 1;}, "(Class<Number>) => number");

    a1.foo();
AssertType(a1.foo(), "void");
AssertType(a1.foo, "() => void");

AssertType(1, "int");
    return 1;
}

// A contextual signature S is extracted from a function type T as follows:
//      If T is a union type, let U be the set of element types in T that have call signatures.
//        If each type in U has exactly one call signature and that call signature is non- generic,
//        and if all of the signatures are identical ignoring return types,
//        then S is a signature with the same parameters and a union of the return types.
let b1: ((s: string, w: boolean) => void) | ((s: string, w: boolean) => string);
AssertType(b1, "union");
AssertType(s, "string");
AssertType(w, "boolean");
AssertType(s, "string");
AssertType(w, "boolean");

b1 = (k, h) => { };
AssertType(b1 = (k, h) => { }, "(string, boolean) => void");
AssertType(b1, "union");
AssertType((k, h) => { }, "(string, boolean) => void");
AssertType(k, "string");
AssertType(h, "boolean");

let b2: typeof a0 | ((n: number, s: string) => string);
AssertType(b2, "union");
AssertType(a0, "(number, string) => number");
AssertType(n, "number");
AssertType(s, "string");

b2 = (foo, bar) => { 
AssertType(b2 = (foo, bar) => { return foo + 1; }, "(number, string) => number");
return foo + 1; 

AssertType(b2, "union");

AssertType((foo, bar) => { return foo + 1; }, "(number, string) => number");

AssertType(foo, "number");

AssertType(bar, "string");

AssertType(foo + 1, "number");

AssertType(foo, "number");

AssertType(1, "int");
}

b2 = (foo, bar) => { 
AssertType(b2 = (foo, bar) => { return "hello"; }, "(number, string) => string");
return "hello"; 

AssertType(b2, "union");

AssertType((foo, bar) => { return "hello"; }, "(number, string) => string");

AssertType(foo, "number");

AssertType(bar, "string");

AssertType("hello", "string");
}

let b3: (name: string, num: number, boo: boolean) => void;
AssertType(b3, "(string, number, boolean) => void");
AssertType(name, "string");
AssertType(num, "number");
AssertType(boo, "boolean");

b3 = (name, number) => { };
AssertType(b3 = (name, number) => { }, "(string, number) => void");
AssertType(b3, "(string, number, boolean) => void");
AssertType((name, number) => { }, "(string, number) => void");
AssertType(name, "string");
AssertType(number, "number");

let b4: (n: E) => string = (number = 1) => { 
AssertType(b4, "(E) => string");
AssertType(n, "E");
AssertType((number = 1) => { return "hello"; }, "(?E) => string");
AssertType(number, "E");
AssertType(1, "int");
AssertType("hello", "string");
return "hello"; };

let b5: (n: {}) => string = (number = "string") => { 
AssertType(b5, "({}) => string");
AssertType(n, "{}");
AssertType((number = "string") => { return "hello"; }, "(?{}) => string");
AssertType(number, "{}");
AssertType("string", "string");
AssertType("hello", "string");
return "hello"; };

// A contextual signature S is extracted from a function type T as follows:
//      Otherwise, no contextual signature can be extracted from T and S is undefined.
let b6: ((s: string, w: boolean) => void) | ((n: number) => number);
AssertType(b6, "union");
AssertType(s, "string");
AssertType(w, "boolean");
AssertType(n, "number");

let b7: ((s: string, w: boolean) => void) | ((s: string, w: number) => string);
AssertType(b7, "union");
AssertType(s, "string");
AssertType(w, "boolean");
AssertType(s, "string");
AssertType(w, "number");

b6 = (k) => { k.toLowerCase() };
AssertType(b6 = (k) => { k.toLowerCase() }, "(any) => void");
AssertType(b6, "union");
AssertType((k) => { k.toLowerCase() }, "(any) => void");
AssertType(k, "any");
AssertType(k.toLowerCase(), "any");
AssertType(k.toLowerCase, "any");

b6 = (i) => {
AssertType(b6, "union");
AssertType((i) => {    i.toExponential();    return i;}, "(any) => any");
AssertType(i, "any");
AssertType(b6 = (i) => {    i.toExponential();    return i;}, "(any) => any");

    i.toExponential();
AssertType(i.toExponential(), "any");
AssertType(i.toExponential, "any");

AssertType(i, "any");
    return i;

};                   // Per spec, no contextual signature can be extracted in this case. (Otherwise clause)
b7 = (j, m) => { };  // Per spec, no contextual signature can be extracted in this case. (Otherwise clause)
AssertType(b7 = (j, m) => { }, "(any, any) => void");
AssertType(b7, "union");
AssertType((j, m) => { }, "(any, any) => void");
AssertType(j, "any");
AssertType(m, "any");

class C<T, U> {
    constructor() {
        let k: ((j: T, k: U) => (T|U)[]) | ((j: number,k :U) => number[]) = (j, k) => {
AssertType(k, "union");
AssertType(j, "T");
AssertType(k, "U");
AssertType(j, "number");
AssertType(k, "U");
AssertType(j, "any");
AssertType(k, "any");
AssertType((j, k) => {            return [j, k];        }, "(any, any) => any[]");

AssertType([j, k], "any[]");
AssertType(j, "any");
AssertType(k, "any");
            return [j, k];

        }   // Per spec, no contextual signature can be extracted in this case.
    }
}

