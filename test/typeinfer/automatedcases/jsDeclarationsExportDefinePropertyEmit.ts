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
Object.defineProperty(module.exports, "a", { value: function a() {} });
AssertType(Object.defineProperty(module.exports, "a", { value: function a() {} }), "typeof module.exports");
AssertType(Object.defineProperty, "<T>(T, PropertyKey, PropertyDescriptor & ThisType<any>) => T");
AssertType(module.exports, "typeof module.exports");
AssertType("a", "string");
AssertType({ value: function a() {} }, "{ value: () => void; }");
AssertType(value, "() => void");
AssertType(function a() {}, "() => void");
AssertType(a, "() => void");

Object.defineProperty(module.exports, "b", { value: function b() {} });
AssertType(Object.defineProperty(module.exports, "b", { value: function b() {} }), "typeof module.exports");
AssertType(Object.defineProperty, "<T>(T, PropertyKey, PropertyDescriptor & ThisType<any>) => T");
AssertType(module.exports, "typeof module.exports");
AssertType("b", "string");
AssertType({ value: function b() {} }, "{ value: () => void; }");
AssertType(value, "() => void");
AssertType(function b() {}, "() => void");
AssertType(b, "() => void");

Object.defineProperty(module.exports.b, "cat", { value: "cat" });
AssertType(Object.defineProperty(module.exports.b, "cat", { value: "cat" }), "() => void");
AssertType(Object.defineProperty, "<T>(T, PropertyKey, PropertyDescriptor & ThisType<any>) => T");
AssertType(module.exports.b, "() => void");
AssertType("cat", "string");
AssertType({ value: "cat" }, "{ value: string; }");
AssertType(value, "string");
AssertType("cat", "string");

/**
 * @param {number} a
 * @param {number} b
 * @
return {string} 
 */
function d(a, b) { 
AssertType((null), "any");
return /** @type {*} */(null); 

AssertType(null, "null");
}

Object.defineProperty(module.exports, "d", { value: d });
AssertType(Object.defineProperty(module.exports, "d", { value: d }), "typeof module.exports");
AssertType(Object.defineProperty, "<T>(T, PropertyKey, PropertyDescriptor & ThisType<any>) => T");
AssertType(module.exports, "typeof module.exports");
AssertType("d", "string");
AssertType({ value: d }, "{ value: (number, number) => string; }");
AssertType(value, "(number, number) => string");
AssertType(d, "(number, number) => string");


/**
 * @template T,U
 * @param {T} a
 * @param {U} b
 * @
return {T & U} 
 */
function e(a, b) { 
AssertType((null), "any");
return /** @type {*} */(null); 

AssertType(null, "null");
}

Object.defineProperty(module.exports, "e", { value: e });
AssertType(Object.defineProperty(module.exports, "e", { value: e }), "typeof module.exports");
AssertType(Object.defineProperty, "<T>(T, PropertyKey, PropertyDescriptor & ThisType<any>) => T");
AssertType(module.exports, "typeof module.exports");
AssertType("e", "string");
AssertType({ value: e }, "{ value: <T, U>(T, U) => T & U; }");
AssertType(value, "<T, U>(T, U) => T & U");
AssertType(e, "<T, U>(T, U) => T & U");

/**
 * @template T
 * @param {T} a
 */
function f(a) {
AssertType(a, "T");
    return a;
}
Object.defineProperty(module.exports, "f", { value: f });
AssertType(Object.defineProperty(module.exports, "f", { value: f }), "typeof module.exports");
AssertType(Object.defineProperty, "<T>(T, PropertyKey, PropertyDescriptor & ThisType<any>) => T");
AssertType(module.exports, "typeof module.exports");
AssertType("f", "string");
AssertType({ value: f }, "{ value: <T>(T) => T; }");
AssertType(value, "<T>(T) => T");
AssertType(f, "<T>(T) => T");

Object.defineProperty(module.exports.f, "self", { value: module.exports.f });
AssertType(Object.defineProperty(module.exports.f, "self", { value: module.exports.f }), "<T>(T) => T");
AssertType(Object.defineProperty, "<T>(T, PropertyKey, PropertyDescriptor & ThisType<any>) => T");
AssertType(module.exports.f, "<T>(T) => T");
AssertType("self", "string");
AssertType({ value: module.exports.f }, "{ value: <T>(T) => T; }");
AssertType(value, "<T>(T) => T");
AssertType(module.exports.f, "<T>(T) => T");

/**
 * @param {{x: string}} a
 * @param {{y: typeof module.exports.b}} b
 */
function g(a, b) {
AssertType(a.x && b.y(), "void");
AssertType(a.x, "string");
AssertType(b.y(), "void");
AssertType(b.y, "() => void");
    return a.x && b.y();
}
Object.defineProperty(module.exports, "g", { value: g });
AssertType(Object.defineProperty(module.exports, "g", { value: g }), "typeof module.exports");
AssertType(Object.defineProperty, "<T>(T, PropertyKey, PropertyDescriptor & ThisType<any>) => T");
AssertType(module.exports, "typeof module.exports");
AssertType("g", "string");
AssertType({ value: g }, "{ value: ({ x: string; }, { y: () => void; }) => void; }");
AssertType(value, "({ x: string; }, { y: () => void; }) => void");
AssertType(g, "({ x: string; }, { y: () => void; }) => void");


/**
 * @param {{x: string}} a
 * @param {{y: typeof module.exports.b}} b
 */
function hh(a, b) {
AssertType(a.x && b.y(), "void");
AssertType(a.x, "string");
AssertType(b.y(), "void");
AssertType(b.y, "() => void");
    return a.x && b.y();
}
Object.defineProperty(module.exports, "h", { value: hh });
AssertType(Object.defineProperty(module.exports, "h", { value: hh }), "typeof module.exports");
AssertType(Object.defineProperty, "<T>(T, PropertyKey, PropertyDescriptor & ThisType<any>) => T");
AssertType(module.exports, "typeof module.exports");
AssertType("h", "string");
AssertType({ value: hh }, "{ value: ({ x: string; }, { y: () => void; }) => void; }");
AssertType(value, "({ x: string; }, { y: () => void; }) => void");
AssertType(hh, "({ x: string; }, { y: () => void; }) => void");

Object.defineProperty(module.exports, "i", { value: function i(){} });
AssertType(Object.defineProperty(module.exports, "i", { value: function i(){} }), "typeof module.exports");
AssertType(Object.defineProperty, "<T>(T, PropertyKey, PropertyDescriptor & ThisType<any>) => T");
AssertType(module.exports, "typeof module.exports");
AssertType("i", "string");
AssertType({ value: function i(){} }, "{ value: () => void; }");
AssertType(value, "() => void");
AssertType(function i(){}, "() => void");
AssertType(i, "() => void");

Object.defineProperty(module.exports, "ii", { value: module.exports.i });
AssertType(Object.defineProperty(module.exports, "ii", { value: module.exports.i }), "typeof module.exports");
AssertType(Object.defineProperty, "<T>(T, PropertyKey, PropertyDescriptor & ThisType<any>) => T");
AssertType(module.exports, "typeof module.exports");
AssertType("ii", "string");
AssertType({ value: module.exports.i }, "{ value: () => void; }");
AssertType(value, "() => void");
AssertType(module.exports.i, "() => void");

// note that this last one doesn't make much sense in cjs, since exports aren't hoisted bindings
Object.defineProperty(module.exports, "jj", { value: module.exports.j });
AssertType(Object.defineProperty(module.exports, "jj", { value: module.exports.j }), "typeof module.exports");
AssertType(Object.defineProperty, "<T>(T, PropertyKey, PropertyDescriptor & ThisType<any>) => T");
AssertType(module.exports, "typeof module.exports");
AssertType("jj", "string");
AssertType({ value: module.exports.j }, "{ value: () => void; }");
AssertType(value, "() => void");
AssertType(module.exports.j, "() => void");

Object.defineProperty(module.exports, "j", { value: function j() {} });
AssertType(Object.defineProperty(module.exports, "j", { value: function j() {} }), "typeof module.exports");
AssertType(Object.defineProperty, "<T>(T, PropertyKey, PropertyDescriptor & ThisType<any>) => T");
AssertType(module.exports, "typeof module.exports");
AssertType("j", "string");
AssertType({ value: function j() {} }, "{ value: () => void; }");
AssertType(value, "() => void");
AssertType(function j() {}, "() => void");
AssertType(j, "() => void");


