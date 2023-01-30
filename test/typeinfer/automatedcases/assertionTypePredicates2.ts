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

// === tests/cases/conformance/controlFlow/assertionTypePredicates2.js ===
declare function AssertType(value:any, type:string):void;
/**
 * @typedef {{ x: number }} A
 */

/**
 * @typedef { A & { y: number } } B
 */

/**
 * @param {A} a
 * @
returns { asserts a is B }
 */
const foo = (a) => {
AssertType(foo, "(A) => asserts a is B");
AssertType(a, "A");
AssertType((a) => {    if (/** @type { B } */ (a).y !== 0) throw TypeError();    return undefined;}, "(A) => asserts a is B");

    if (/** @type { B } */ (a).y !== 0) throw TypeError();
AssertType((a).y !== 0, "boolean");
AssertType((a).y, "number");
AssertType((a), "B");
AssertType(a, "A");
AssertType(0, "int");
AssertType(TypeError(), "TypeError");
AssertType(TypeError, "TypeErrorConstructor");

AssertType(undefined, "undefined");
    return undefined;

};

export const main = () => {
AssertType(main, "() => void");
AssertType(() => {    /** @type { A } */    const a = { x: 1 };    foo(a);}, "() => void");

    /** @type { A } */
    const a = { x: 1 };
AssertType(a, "A");
AssertType({ x: 1 }, "{ x: number; }");
AssertType(x, "number");
AssertType(1, "int");

    foo(a);
AssertType(foo(a), "void");
AssertType(foo, "(A) => asserts a is B");
AssertType(a, "A");

};


