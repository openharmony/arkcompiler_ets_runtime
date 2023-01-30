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

// === tests/cases/conformance/jsdoc/0.js ===
declare function AssertType(value:any, type:string):void;
// @ts-check
/**
 * @typedef {Object} Opts
 * @property {string} x
 * @property {string=} y
 * @property {string} [z]
 * @property {string} [w="hi"]
 *
 * @param {Opts} opts
 */
function foo(opts) {
    opts.x;
AssertType(opts.x, "string");
}

foo({x: 'abc'});
AssertType(foo({x: 'abc'}), "void");
AssertType(foo, "(Opts) => void");
AssertType({x: 'abc'}, "{ x: string; }");
AssertType(x, "string");
AssertType('abc', "string");

/**
 * @typedef {Object} AnotherOpts
 * @property anotherX {string}
 * @property anotherY {string=}
 * 
 * @param {AnotherOpts} opts
 */
function foo1(opts) {
    opts.anotherX;
AssertType(opts.anotherX, "string");
}

foo1({anotherX: "world"});
AssertType(foo1({anotherX: "world"}), "void");
AssertType(foo1, "(AnotherOpts) => void");
AssertType({anotherX: "world"}, "{ anotherX: string; }");
AssertType(anotherX, "string");
AssertType("world", "string");

/**
 * @typedef {object} Opts1
 * @property {string} x
 * @property {string=} y
 * @property {string} [z]
 * @property {string} [w="hi"]
 *
 * @param {Opts1} opts
 */
function foo2(opts) {
    opts.x;
AssertType(opts.x, "string");
}
foo2({x: 'abc'});
AssertType(foo2({x: 'abc'}), "void");
AssertType(foo2, "(Opts1) => void");
AssertType({x: 'abc'}, "{ x: string; }");
AssertType(x, "string");
AssertType('abc', "string");


