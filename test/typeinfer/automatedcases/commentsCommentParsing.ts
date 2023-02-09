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

// === tests/cases/compiler/commentsCommentParsing.ts ===
declare function AssertType(value:any, type:string):void;
/// This is simple /// comments
function simple() {
}

simple();
AssertType(simple(), "void");
AssertType(simple, "() => void");

/// multiLine /// Comments
/// This is example of multiline /// comments
/// Another multiLine
function multiLine() {
}
multiLine();
AssertType(multiLine(), "void");
AssertType(multiLine, "() => void");

/** this is eg of single line jsdoc style comment */
function jsDocSingleLine() {
}
jsDocSingleLine();
AssertType(jsDocSingleLine(), "void");
AssertType(jsDocSingleLine, "() => void");


/** this is multiple line jsdoc stule comment
*New line1
*New Line2*/
function jsDocMultiLine() {
}
jsDocMultiLine();
AssertType(jsDocMultiLine(), "void");
AssertType(jsDocMultiLine, "() => void");

/** this is multiple line jsdoc stule comment
*New line1
*New Line2*/
/** Shoul mege this line as well
* and this too*/ /** Another this one too*/
function jsDocMultiLineMerge() {
}
jsDocMultiLineMerge();
AssertType(jsDocMultiLineMerge(), "void");
AssertType(jsDocMultiLineMerge, "() => void");


/// Triple slash comment
/** jsdoc comment */
function jsDocMixedComments1() {
}
jsDocMixedComments1();
AssertType(jsDocMixedComments1(), "void");
AssertType(jsDocMixedComments1, "() => void");

/// Triple slash comment
/** jsdoc comment */ /*** another jsDocComment*/
function jsDocMixedComments2() {
}
jsDocMixedComments2();
AssertType(jsDocMixedComments2(), "void");
AssertType(jsDocMixedComments2, "() => void");

/** jsdoc comment */ /*** another jsDocComment*/
/// Triple slash comment
function jsDocMixedComments3() {
}
jsDocMixedComments3();
AssertType(jsDocMixedComments3(), "void");
AssertType(jsDocMixedComments3, "() => void");

/** jsdoc comment */ /*** another jsDocComment*/
/// Triple slash comment
/// Triple slash comment 2
function jsDocMixedComments4() {
}
jsDocMixedComments4();
AssertType(jsDocMixedComments4(), "void");
AssertType(jsDocMixedComments4, "() => void");

/// Triple slash comment 1
/** jsdoc comment */ /*** another jsDocComment*/
/// Triple slash comment
/// Triple slash comment 2
function jsDocMixedComments5() {
}
jsDocMixedComments5();
AssertType(jsDocMixedComments5(), "void");
AssertType(jsDocMixedComments5, "() => void");

/*** another jsDocComment*/
/// Triple slash comment 1
/// Triple slash comment
/// Triple slash comment 2
/** jsdoc comment */
function jsDocMixedComments6() {
}
jsDocMixedComments6();
AssertType(jsDocMixedComments6(), "void");
AssertType(jsDocMixedComments6, "() => void");

// This shoulnot be help comment
function noHelpComment1() {
}
noHelpComment1();
AssertType(noHelpComment1(), "void");
AssertType(noHelpComment1, "() => void");

/* This shoulnot be help comment */
function noHelpComment2() {
}
noHelpComment2();
AssertType(noHelpComment2(), "void");
AssertType(noHelpComment2, "() => void");

function noHelpComment3() {
}
noHelpComment3();
AssertType(noHelpComment3(), "void");
AssertType(noHelpComment3, "() => void");

/** Adds two integers and 
returns the result
  * @param {number} a first number
  * @param b second number
  */
function sum(a: number, b: number) {
AssertType(a + b, "number");
AssertType(a, "number");
AssertType(b, "number");
    return a + b;
}
sum(10, 20);
AssertType(sum(10, 20), "number");
AssertType(sum, "(number, number) => number");
AssertType(10, "int");
AssertType(20, "int");

/** This is multiplication function*/
/** @param */
/** @param a first number*/
/** @param b */
/** @param c {
 @param d @anotherTag*/
/** @param e LastParam @anotherTag*/
function multiply(a: number, b: number, c?: number, d?, e?) {
}
/** fn f1 with number
* @param { string} b about b
*/
function f1(a: number);
function f1(b: string);
/**@param opt optional parameter*/
function f1(aOrb, opt?) {
AssertType(aOrb, "any");
    return aOrb;
}
/** This is subtract function
@param { a
*@param { number | } b this is about b
@param { { () => string; } } c this is optional param c
@param { { () => string; } d this is optional param d
@param { { () => string; } } e this is optional param e
@param { { { () => string; } } f this is optional param f
*/
function subtract(a: number, b: number, c?: () => string, d?: () => string, e?: () => string, f?: () => string) {
}
/** this is square function
@paramTag { number } a this is input number of paramTag
@param { number } a this is input number
@
returnType { number } it is return type
*/
function square(a: number) {
AssertType(a * a, "number");
AssertType(a, "number");
AssertType(a, "number");
    return a * a;
}
/** this is divide function
@param { number} a this is a
@paramTag { number } g this is optional param g
@param { number} b this is b
*/
function divide(a: number, b: number) {
}
/** this is jsdoc style function with param tag as well as inline parameter help
*@param a it is first parameter
*@param c it is third parameter
*/
function jsDocParamTest(/** this is inline comment for a */a: number, /** this is inline comment for b*/ b: number, c: number, d: number) {
AssertType(a + b + c + d, "number");
AssertType(a + b + c, "number");
AssertType(a + b, "number");
AssertType(a, "number");
AssertType(b, "number");
AssertType(c, "number");
AssertType(d, "number");
    return a + b + c + d;
}

/**/
class NoQuickInfoClass {
}

