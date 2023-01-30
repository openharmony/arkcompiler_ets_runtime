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

// === tests/cases/compiler/funcdecl.ts ===
declare function AssertType(value:any, type:string):void;
function simpleFunc() {
AssertType("this is my simple func", "string");
    return "this is my simple func";
}
let simpleFuncVar = simpleFunc;
AssertType(simpleFuncVar, "() => string");
AssertType(simpleFunc, "() => string");

function anotherFuncNoReturn() {
}
let anotherFuncNoReturnVar = anotherFuncNoReturn;
AssertType(anotherFuncNoReturnVar, "() => void");
AssertType(anotherFuncNoReturn, "() => void");

function withReturn() : string{
AssertType("Hello", "string");
    return "Hello";
}
let withReturnVar = withReturn;
AssertType(withReturnVar, "() => string");
AssertType(withReturn, "() => string");

function withParams(a : string) : string{
AssertType(a, "string");
    return a;
}
let withparamsVar = withParams;
AssertType(withparamsVar, "(string) => string");
AssertType(withParams, "(string) => string");

function withMultiParams(a : number, b, c: Object) {
AssertType(a, "number");
    return a;
}
let withMultiParamsVar = withMultiParams;
AssertType(withMultiParamsVar, "(number, any, Object) => number");
AssertType(withMultiParams, "(number, any, Object) => number");

function withOptionalParams(a?: string) {
}
let withOptionalParamsVar = withOptionalParams;
AssertType(withOptionalParamsVar, "(?string) => void");
AssertType(withOptionalParams, "(?string) => void");

function withInitializedParams(a: string, b0, b = 30, c = "string value") {
}
let withInitializedParamsVar = withInitializedParams;
AssertType(withInitializedParamsVar, "(string, any, ?number, ?string) => void");
AssertType(withInitializedParams, "(string, any, ?number, ?string) => void");

function withOptionalInitializedParams(a: string, c: string = "hello string") {
}
let withOptionalInitializedParamsVar = withOptionalInitializedParams;
AssertType(withOptionalInitializedParamsVar, "(string, ?string) => void");
AssertType(withOptionalInitializedParams, "(string, ?string) => void");

function withRestParams(a: string, ... myRestParameter : number[]) {
AssertType(myRestParameter, "number[]");
    return myRestParameter;
}
let withRestParamsVar = withRestParams;
AssertType(withRestParamsVar, "(string, ...number[]) => number[]");
AssertType(withRestParams, "(string, ...number[]) => number[]");

function overload1(n: number) : string;
function overload1(s: string) : string;
function overload1(ns: any) {
AssertType(ns.toString(), "any");
AssertType(ns.toString, "any");
    return ns.toString();
}
let withOverloadSignature = overload1;
AssertType(withOverloadSignature, "{ (number): string; (string): string; }");
AssertType(overload1, "{ (number): string; (string): string; }");

function f(n: () => void) { }

module m2 {
    export function foo(n: () => void ) {
    }

}

m2.foo(() =>  {
AssertType(m2.foo, "(() => void) => void");
AssertType(() =>  {    let b = 30;    return b;}, "() => number");
AssertType(m2.foo(() =>  {    let b = 30;    return b;}), "void");

    let b = 30;
AssertType(b, "number");
AssertType(30, "int");

AssertType(b, "number");
    return b;

});


declare function fooAmbient(n: number): string;

declare function overloadAmbient(n: number): string;
declare function overloadAmbient(s: string): string;

let f2 = () => {
AssertType(f2, "() => string");
AssertType(() => {    return "string";}, "() => string");

AssertType("string", "string");
    return "string";
}

