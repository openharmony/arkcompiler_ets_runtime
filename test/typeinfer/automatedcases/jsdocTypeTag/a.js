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

// === tests/cases/conformance/jsdoc/a.js ===
declare function AssertType(value:any, type:string):void;
/** @type {String} */
let S;
AssertType(S, "string");

/** @type {string} */
let s;
AssertType(s, "string");

/** @type {Number} */
let N;
AssertType(N, "number");

/** @type {number} */
let n;
AssertType(n, "number");

/** @type {Boolean} */
let B;
AssertType(B, "boolean");

/** @type {boolean} */
let b;
AssertType(b, "boolean");

/** @type {Void} */
let V;
AssertType(V, "void");

/** @type {void} */
let v;
AssertType(v, "void");

/** @type {Undefined} */
let U;
AssertType(U, "undefined");

/** @type {undefined} */
let u;
AssertType(u, "undefined");

/** @type {Null} */
let Nl;
AssertType(Nl, "null");

/** @type {null} */
let nl;
AssertType(nl, "null");

/** @type {Array} */
let A;
AssertType(A, "any[]");

/** @type {array} */
let a;
AssertType(a, "any[]");

/** @type {Promise} */
let P;
AssertType(P, "Promise<any>");

/** @type {promise} */
let p;
AssertType(p, "Promise<any>");

/** @type {?number} */
let nullable;
AssertType(nullable, "union");

/** @type {Object} */
let Obj;
AssertType(Obj, "any");

/** @type {object} */
let obj;
AssertType(obj, "any");

/** @type {Function} */
let Func;
AssertType(Func, "Function");

/** @type {(s: string) => number} */
let f;
AssertType(f, "(string) => number");

/** @type {new (s: string) => { s: string }} */
let ctor;
AssertType(ctor, "new (string) => { s: string; }");


