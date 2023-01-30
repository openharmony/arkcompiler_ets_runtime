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

// === tests/cases/compiler/commentsFunction.ts ===
declare function AssertType(value:any, type:string):void;
/** This comment should appear for foo*/
function foo() {
} /* trailing comment of function */
foo();
AssertType(foo(), "void");
AssertType(foo, "() => void");

/** This is comment for function signature*/
function fooWithParameters(/** this is comment about a*/a: string,
    /** this is comment for b*/
    b: number) {
    let d = a;
AssertType(d, "string");
AssertType(a, "string");

} // trailing comment of function
fooWithParameters("a", 10);
AssertType(fooWithParameters("a", 10), "void");
AssertType(fooWithParameters, "(string, number) => void");
AssertType("a", "string");
AssertType(10, "int");

/** fooFunc
 * comment
 */
let fooFunc = function FooFunctionValue(/** fooFunctionValue param */ b: string) {
AssertType(fooFunc, "(string) => string");
AssertType(FooFunctionValue, "(string) => string");
AssertType(b, "string");
AssertType(function FooFunctionValue(/** fooFunctionValue param */ b: string) {    return b;}, "(string) => string");

AssertType(b, "string");
    return b;
}

/// lamdaFoo let comment
let lambdaFoo = /** this is lambda comment*/ (/**param a*/a: number, /**param b*/b: number) => a + b;
AssertType(lambdaFoo, "(number, number) => number");
AssertType((/**param a*/a: number, /**param b*/b: number) => a + b, "(number, number) => number");
AssertType(a, "number");
AssertType(b, "number");
AssertType(a + b, "number");
AssertType(a, "number");
AssertType(b, "number");

let lambddaNoVarComment = /** this is lambda multiplication*/ (/**param a*/a: number, /**param b*/b: number) => a * b;
AssertType(lambddaNoVarComment, "(number, number) => number");
AssertType((/**param a*/a: number, /**param b*/b: number) => a * b, "(number, number) => number");
AssertType(a, "number");
AssertType(b, "number");
AssertType(a * b, "number");
AssertType(a, "number");
AssertType(b, "number");

lambdaFoo(10, 20);
AssertType(lambdaFoo(10, 20), "number");
AssertType(lambdaFoo, "(number, number) => number");
AssertType(10, "int");
AssertType(20, "int");

lambddaNoVarComment(10, 20);
AssertType(lambddaNoVarComment(10, 20), "number");
AssertType(lambddaNoVarComment, "(number, number) => number");
AssertType(10, "int");
AssertType(20, "int");

function blah(a: string /* multiline trailing comment 
multiline */) {
}

function blah2(a: string /* single line multiple trailing comments */ /* second */) {
}

function blah3(a: string // trailing commen single line
    ) {
}

lambdaFoo = (a, b) => a * b; // This is trailing comment
AssertType(lambdaFoo = (a, b) => a * b, "(number, number) => number");
AssertType(lambdaFoo, "(number, number) => number");
AssertType((a, b) => a * b, "(number, number) => number");
AssertType(a, "number");
AssertType(b, "number");
AssertType(a * b, "number");
AssertType(a, "number");
AssertType(b, "number");

/*leading comment*/() => 0; // Needs to be wrapped in parens to be a valid expression (not declaration)
AssertType(() => 0, "() => number");
AssertType(0, "int");

/*leading comment*/(() => 0); //trailing comment
AssertType((() => 0), "() => number");
AssertType(() => 0, "() => number");
AssertType(0, "int");

function blah4(/*1*/a: string/*2*/,/*3*/b: string/*4*/) {
}

function foo1() {

    // should emit this
}

function foo2() {
    /// This is some detached comment

    // should emit this leading comment of } too
}


