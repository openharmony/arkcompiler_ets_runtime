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
// these are recognized as TS concepts by the checker
/** @type {String} */const a = "";
AssertType(a, "string");
AssertType("", "string");

/** @type {Number} */const b = 0;
AssertType(b, "number");
AssertType(0, "int");

/** @type {Boolean} */const c = true;
AssertType(c, "boolean");
AssertType(true, "boolean");

/** @type {Void} */const d = undefined;
AssertType(d, "void");
AssertType(undefined, "undefined");

/** @type {Undefined} */const e = undefined;
AssertType(e, "undefined");
AssertType(undefined, "undefined");

/** @type {Null} */const f = null;
AssertType(f, "null");
AssertType(null, "null");

/** @type {Function} */const g = () => void 0;
AssertType(g, "Function");
AssertType(() => void 0, "() => undefined");
AssertType(void 0, "undefined");
AssertType(0, "int");

/** @type {function} */const h = () => void 0;
AssertType(h, "Function");
AssertType(() => void 0, "() => undefined");
AssertType(void 0, "undefined");
AssertType(0, "int");

/** @type {array} */const i = [];
AssertType(i, "any[]");
AssertType([], "never[]");

/** @type {promise} */const j = Promise.resolve(0);
AssertType(j, "Promise<any>");
AssertType(Promise.resolve(0), "Promise<number>");
AssertType(Promise.resolve, "{ (): Promise<void>; <T>(T): Promise<Awaited<T>>; <T>(union): Promise<Awaited<T>>; }");
AssertType(0, "int");

/** @type {Object<string, string>} */const k = {x: "x"};
AssertType(k, "{ [string]: string; }");
AssertType({x: "x"}, "{ x: string; }");
AssertType(x, "string");
AssertType("x", "string");


// these are not recognized as anything and should just be lookup failures
// ignore the errors to try to ensure they're emitted as `any` in declaration emit
// @ts-ignore
/** @type {class} */const l = true;
AssertType(l, "error");
AssertType(true, "boolean");

// @ts-ignore
/** @type {bool} */const m = true;
AssertType(m, "error");
AssertType(true, "boolean");

// @ts-ignore
/** @type {int} */const n = true;
AssertType(n, "error");
AssertType(true, "boolean");

// @ts-ignore
/** @type {float} */const o = true;
AssertType(o, "error");
AssertType(true, "boolean");

// @ts-ignore
/** @type {integer} */const p = true;
AssertType(p, "error");
AssertType(true, "boolean");

// or, in the case of `event` likely erroneously refers to the type of the global Event object
/** @type {event} */const q = undefined;
AssertType(q, "union");
AssertType(undefined, "undefined");


