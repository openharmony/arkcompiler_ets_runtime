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

// === tests/cases/conformance/expressions/contextualTyping/parenthesizedContexualTyping3.ts ===
declare function AssertType(value:any, type:string):void;
// Contextual typing for parenthesized substitution expressions in tagged templates.

/**
 * tempFun - Can't have fun for too long.
 */
function tempFun<T>(tempStrs: TemplateStringsArray, g: (x: T) => T, x: T): T;
function tempFun<T>(tempStrs: TemplateStringsArray, g: (x: T) => T, h: (y: T) => T, x: T): T;
function tempFun<T>(tempStrs: TemplateStringsArray, g: (x: T) => T, x: T): T {
AssertType(g(x), "T");
AssertType(g, "(T) => T");
AssertType(x, "T");
    return g(x);
}

let a = tempFun `${ x => x }  ${ 10 }`
AssertType(a, "number");
AssertType(tempFun `${ x => x }  ${ 10 }`, "number");
AssertType(tempFun, "{ <T>(TemplateStringsArray, (T) => T, T): T; <T>(TemplateStringsArray, (T) => T, (T) => T, T): T; }");
AssertType(`${ x => x }  ${ 10 }`, "string");
AssertType(x => x, "(number) => number");
AssertType(x, "number");
AssertType(x, "number");
AssertType(10, "int");

let b = tempFun `${ (x => x) }  ${ 10 }`
AssertType(b, "number");
AssertType(tempFun `${ (x => x) }  ${ 10 }`, "number");
AssertType(tempFun, "{ <T>(TemplateStringsArray, (T) => T, T): T; <T>(TemplateStringsArray, (T) => T, (T) => T, T): T; }");
AssertType(`${ (x => x) }  ${ 10 }`, "string");
AssertType((x => x), "(number) => number");
AssertType(x => x, "(number) => number");
AssertType(x, "number");
AssertType(x, "number");
AssertType(10, "int");

let c = tempFun `${ ((x => x)) } ${ 10 }`
AssertType(c, "number");
AssertType(tempFun `${ ((x => x)) } ${ 10 }`, "number");
AssertType(tempFun, "{ <T>(TemplateStringsArray, (T) => T, T): T; <T>(TemplateStringsArray, (T) => T, (T) => T, T): T; }");
AssertType(`${ ((x => x)) } ${ 10 }`, "string");
AssertType(((x => x)), "(number) => number");
AssertType((x => x), "(number) => number");
AssertType(x => x, "(number) => number");
AssertType(x, "number");
AssertType(x, "number");
AssertType(10, "int");

let d = tempFun `${ x => x } ${ x => x } ${ 10 }`
AssertType(d, "number");
AssertType(tempFun `${ x => x } ${ x => x } ${ 10 }`, "number");
AssertType(tempFun, "{ <T>(TemplateStringsArray, (T) => T, T): T; <T>(TemplateStringsArray, (T) => T, (T) => T, T): T; }");
AssertType(`${ x => x } ${ x => x } ${ 10 }`, "string");
AssertType(x => x, "(number) => number");
AssertType(x, "number");
AssertType(x, "number");
AssertType(x => x, "(number) => number");
AssertType(x, "number");
AssertType(x, "number");
AssertType(10, "int");

let e = tempFun `${ x => x } ${ (x => x) } ${ 10 }`
AssertType(e, "number");
AssertType(tempFun `${ x => x } ${ (x => x) } ${ 10 }`, "number");
AssertType(tempFun, "{ <T>(TemplateStringsArray, (T) => T, T): T; <T>(TemplateStringsArray, (T) => T, (T) => T, T): T; }");
AssertType(`${ x => x } ${ (x => x) } ${ 10 }`, "string");
AssertType(x => x, "(number) => number");
AssertType(x, "number");
AssertType(x, "number");
AssertType((x => x), "(number) => number");
AssertType(x => x, "(number) => number");
AssertType(x, "number");
AssertType(x, "number");
AssertType(10, "int");

let f = tempFun `${ x => x } ${ ((x => x)) } ${ 10 }`
AssertType(f, "number");
AssertType(tempFun `${ x => x } ${ ((x => x)) } ${ 10 }`, "number");
AssertType(tempFun, "{ <T>(TemplateStringsArray, (T) => T, T): T; <T>(TemplateStringsArray, (T) => T, (T) => T, T): T; }");
AssertType(`${ x => x } ${ ((x => x)) } ${ 10 }`, "string");
AssertType(x => x, "(number) => number");
AssertType(x, "number");
AssertType(x, "number");
AssertType(((x => x)), "(number) => number");
AssertType((x => x), "(number) => number");
AssertType(x => x, "(number) => number");
AssertType(x, "number");
AssertType(x, "number");
AssertType(10, "int");

let g = tempFun `${ (x => x) } ${ (((x => x))) } ${ 10 }`
AssertType(g, "number");
AssertType(tempFun `${ (x => x) } ${ (((x => x))) } ${ 10 }`, "number");
AssertType(tempFun, "{ <T>(TemplateStringsArray, (T) => T, T): T; <T>(TemplateStringsArray, (T) => T, (T) => T, T): T; }");
AssertType(`${ (x => x) } ${ (((x => x))) } ${ 10 }`, "string");
AssertType((x => x), "(number) => number");
AssertType(x => x, "(number) => number");
AssertType(x, "number");
AssertType(x, "number");
AssertType((((x => x))), "(number) => number");
AssertType(((x => x)), "(number) => number");
AssertType((x => x), "(number) => number");
AssertType(x => x, "(number) => number");
AssertType(x, "number");
AssertType(x, "number");
AssertType(10, "int");

let h = tempFun `${ (x => x) } ${ (((x => x))) } ${ undefined }`
AssertType(h, "any");
AssertType(tempFun `${ (x => x) } ${ (((x => x))) } ${ undefined }`, "any");
AssertType(tempFun, "{ <T>(TemplateStringsArray, (T) => T, T): T; <T>(TemplateStringsArray, (T) => T, (T) => T, T): T; }");
AssertType(`${ (x => x) } ${ (((x => x))) } ${ undefined }`, "string");
AssertType((x => x), "(any) => any");
AssertType(x => x, "(any) => any");
AssertType(x, "any");
AssertType(x, "any");
AssertType((((x => x))), "(any) => any");
AssertType(((x => x)), "(any) => any");
AssertType((x => x), "(any) => any");
AssertType(x => x, "(any) => any");
AssertType(x, "any");
AssertType(x, "any");
AssertType(undefined, "undefined");


