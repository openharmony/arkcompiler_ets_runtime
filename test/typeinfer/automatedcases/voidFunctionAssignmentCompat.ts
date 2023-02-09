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

// === tests/cases/compiler/voidFunctionAssignmentCompat.ts ===
declare function AssertType(value:any, type:string):void;
let fa = function(): any { 
AssertType(fa, "() => any");
return 3; 

AssertType(function(): any { return 3; }, "() => any");

AssertType(3, "int");
}

fa = function() { } // should not work
AssertType(fa = function() { }, "() => void");
AssertType(fa, "() => any");
AssertType(function() { }, "() => void");

let fv = function(): void {
AssertType(fv, "() => void");

AssertType(function(): void {}, "() => void");
}

fv = function() { 
AssertType(fv = function() { return 0; }, "() => number");
AssertType(fv, "() => void");
AssertType(function() { return 0; }, "() => number");
AssertType(0, "int");
return 0; } // should work

function execAny(callback:(val:any)=>any) { 
AssertType(callback(0), "any");
return callback(0) 

AssertType(callback, "(any) => any");

AssertType(0, "int");
}

execAny(function () {}); // should work
AssertType(execAny(function () {}), "any");
AssertType(execAny, "((any) => any) => any");
AssertType(function () {}, "() => void");

function execVoid(callback:(val:any)=>void) { callback(0); 
AssertType(callback(0), "void");

AssertType(callback, "(any) => void");

AssertType(0, "int");
}

execVoid(function () {
AssertType(execVoid(function () {return 0;}), "void");
AssertType(execVoid, "((any) => void) => void");
AssertType(function () {return 0;}, "() => number");
AssertType(0, "int");
return 0;}); // should work

let fra: (v:any)=>any = function() { 
AssertType(fra, "(any) => any");
AssertType(v, "any");
AssertType(function() { return function () {}; }, "() => () => void");
AssertType(function () {}, "() => void");
return function () {}; } // should work

let frv: (v:any)=>void = function() { 
AssertType(frv, "(any) => void");
AssertType(v, "any");
AssertType(function() { return function () { return 0; } }, "() => () => number");
AssertType(function () { return 0; }, "() => number");
AssertType(0, "int");
return function () { return 0; } } // should work

let fra3: (v:any)=>string = (function() { 
AssertType(fra3, "(any) => string");
AssertType(v, "any");
AssertType((function() { return function (v:string) {return v;}; })(), "(string) => string");
AssertType((function() { return function (v:string) {return v;}; }), "() => (string) => string");
AssertType(function() { return function (v:string) {return v;}; }, "() => (string) => string");
AssertType(function (v:string) {return v;}, "(string) => string");
AssertType(v, "string");
AssertType(v, "string");
return function (v:string) {return v;}; })() // should work

let frv3: (v:any)=>number = (function() { 
AssertType(frv3, "(any) => number");
AssertType(v, "any");
AssertType((function() { return function () { return 0; } })(), "() => number");
AssertType((function() { return function () { return 0; } }), "() => () => number");
AssertType(function() { return function () { return 0; } }, "() => () => number");
AssertType(function () { return 0; }, "() => number");
AssertType(0, "int");
return function () { return 0; } })() // should work



