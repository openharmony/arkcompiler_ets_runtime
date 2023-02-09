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
export function a() {}

export function b() {}
b.cat = "cat";
AssertType(b.cat = "cat", "string");
AssertType(b.cat, "string");
AssertType("cat", "string");

export function c() {}
c.Cls = class {
AssertType(c.Cls = class {}, "typeof Cls");

AssertType(c.Cls, "typeof Cls");

AssertType(class {}, "typeof Cls");
}

/**
 * @param {number} a
 * @param {number} b
 * @
return {string} 
 */
export function d(a, b) { 
AssertType((null), "any");
return /** @type {*} */(null); 

AssertType(null, "null");
}

/**
 * @template T,U
 * @param {T} a
 * @param {U} b
 * @
return {T & U} 
 */
export function e(a, b) { 
AssertType((null), "any");
return /** @type {*} */(null); 

AssertType(null, "null");
}

/**
 * @template T
 * @param {T} a
 */
export function f(a) {
AssertType(a, "T");
    return a;
}
f.self = f;
AssertType(f.self = f, "typeof f");
AssertType(f.self, "typeof f");
AssertType(f, "typeof f");

/**
 * @param {{x: string}} a
 * @param {{y: typeof b}} b
 */
function g(a, b) {
AssertType(a.x && b.y(), "void");
AssertType(a.x, "string");
AssertType(b.y(), "void");
AssertType(b.y, "typeof import("tests/cases/conformance/jsdoc/declarations/index").b");
    return a.x && b.y();
}

export { g };

/**
 * @param {{x: string}} a
 * @param {{y: typeof b}} b
 */
function hh(a, b) {
AssertType(a.x && b.y(), "void");
AssertType(a.x, "string");
AssertType(b.y(), "void");
AssertType(b.y, "typeof import("tests/cases/conformance/jsdoc/declarations/index").b");
    return a.x && b.y();
}

export { hh as h };

export function i() {}
export { i as ii };

export { j as jj };
export function j() {}


