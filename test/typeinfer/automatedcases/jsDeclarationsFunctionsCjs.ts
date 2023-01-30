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

// === tests/cases/conformance/jsdoc/declarations/index.js ===
declare function AssertType(value:any, type:string):void;
module.exports.a = function a() {
AssertType(module.exports.a = function a() {}, "() => void");

AssertType(module.exports.a, "() => void");

AssertType(function a() {}, "() => void");

AssertType(a, "() => void");
}

module.exports.b = function b() {
AssertType(module.exports.b = function b() {}, "{ (): void; cat: string; }");

AssertType(module.exports.b, "{ (): void; cat: string; }");

AssertType(function b() {}, "{ (): void; cat: string; }");

AssertType(b, "{ (): void; cat: string; }");
}

module.exports.b.cat = "cat";
AssertType(module.exports.b.cat = "cat", "string");
AssertType(module.exports.b.cat, "string");
AssertType("cat", "string");

module.exports.c = function c() {
AssertType(module.exports.c = function c() {}, "{ (): void; Cls: typeof Cls; }");

AssertType(module.exports.c, "{ (): void; Cls: typeof Cls; }");

AssertType(function c() {}, "{ (): void; Cls: typeof Cls; }");

AssertType(c, "{ (): void; Cls: typeof Cls; }");
}

module.exports.c.Cls = class {
AssertType(module.exports.c.Cls = class {}, "typeof Cls");

AssertType(module.exports.c.Cls, "typeof Cls");

AssertType(class {}, "typeof Cls");
}

/**
 * @param {number} a
 * @param {number} b
 * @
return {string} 
 */
module.exports.d = function d(a, b) { 
AssertType(module.exports.d = function d(a, b) { return /** @type {*} */(null); }, "(number, number) => string");
return /** @type {*} */(null); 

AssertType(module.exports.d, "(number, number) => string");

AssertType(function d(a, b) { return /** @type {*} */(null); }, "(number, number) => string");

AssertType(d, "(number, number) => string");

AssertType(a, "number");

AssertType(b, "number");

AssertType((null), "any");

AssertType(null, "null");
}

/**
 * @template T,U
 * @param {T} a
 * @param {U} b
 * @
return {T & U} 
 */
module.exports.e = function e(a, b) { 
AssertType(module.exports.e = function e(a, b) { return /** @type {*} */(null); }, "<T, U>(T, U) => T & U");
return /** @type {*} */(null); 

AssertType(module.exports.e, "<T, U>(T, U) => T & U");

AssertType(function e(a, b) { return /** @type {*} */(null); }, "<T, U>(T, U) => T & U");

AssertType(e, "<T, U>(T, U) => T & U");

AssertType(a, "T");

AssertType(b, "U");

AssertType((null), "any");

AssertType(null, "null");
}

/**
 * @template T
 * @param {T} a
 */
module.exports.f = function f(a) {
AssertType(module.exports.f, "{ <T>(T): T; self: any; }");
AssertType(function f(a) {    return a;}, "{ <T>(T): T; self: any; }");
AssertType(f, "{ <T>(T): T; self: any; }");
AssertType(a, "T");
AssertType(module.exports.f = function f(a) {    return a;}, "{ <T>(T): T; self: any; }");

AssertType(a, "T");
    return a;
}
module.exports.f.self = module.exports.f;
AssertType(module.exports.f.self = module.exports.f, "{ <T>(T): T; self: any; }");
AssertType(module.exports.f.self, "{ <T>(T): T; self: any; }");
AssertType(module.exports.f, "{ <T>(T): T; self: any; }");

/**
 * @param {{x: string}} a
 * @param {{y: typeof module.exports.b}} b
 */
function g(a, b) {
AssertType(a.x && b.y(), "void");
AssertType(a.x, "string");
AssertType(b.y(), "void");
AssertType(b.y, "{ (): void; cat: string; }");
    return a.x && b.y();
}

module.exports.g = g;
AssertType(module.exports.g = g, "({ x: string; }, { y: { (): void; cat: string; }; }) => void");
AssertType(module.exports.g, "({ x: string; }, { y: { (): void; cat: string; }; }) => void");
AssertType(g, "({ x: string; }, { y: { (): void; cat: string; }; }) => void");

/**
 * @param {{x: string}} a
 * @param {{y: typeof module.exports.b}} b
 */
function hh(a, b) {
AssertType(a.x && b.y(), "void");
AssertType(a.x, "string");
AssertType(b.y(), "void");
AssertType(b.y, "{ (): void; cat: string; }");
    return a.x && b.y();
}

module.exports.h = hh;
AssertType(module.exports.h = hh, "({ x: string; }, { y: { (): void; cat: string; }; }) => void");
AssertType(module.exports.h, "({ x: string; }, { y: { (): void; cat: string; }; }) => void");
AssertType(hh, "({ x: string; }, { y: { (): void; cat: string; }; }) => void");

module.exports.i = function i() {
AssertType(module.exports.i = function i() {}, "() => void");

AssertType(module.exports.i, "() => void");

AssertType(function i() {}, "() => void");

AssertType(i, "() => void");
}

module.exports.ii = module.exports.i;
AssertType(module.exports.ii = module.exports.i, "() => void");
AssertType(module.exports.ii, "() => void");
AssertType(module.exports.i, "() => void");

// note that this last one doesn't make much sense in cjs, since exports aren't hoisted bindings
module.exports.jj = module.exports.j;
AssertType(module.exports.jj = module.exports.j, "() => void");
AssertType(module.exports.jj, "() => void");
AssertType(module.exports.j, "() => void");

module.exports.j = function j() {
AssertType(module.exports.j = function j() {}, "() => void");

AssertType(module.exports.j, "() => void");

AssertType(function j() {}, "() => void");

AssertType(j, "() => void");
}


