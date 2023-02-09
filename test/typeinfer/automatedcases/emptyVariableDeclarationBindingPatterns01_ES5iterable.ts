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

// === tests/cases/conformance/es6/destructuring/emptyVariableDeclarationBindingPatterns01_ES5iterable.ts ===
declare function AssertType(value:any, type:string):void;
(function () {
AssertType((function () {    let a: any;    let {} = a;    let {} = a;    const {} = a;    let [] = a;    let [] = a;    const [] = a;    let {} = a, [] = a;    let {} = a, [] = a;    const {} = a, [] = a;    let { p1: {}, p2: [] } = a;    let { p1: {}, p2: [] } = a;    const { p1: {}, p2: [] } = a;    for (let {} = {}, {} = {}; false; void 0) {    }    function f({} = a, [] = a, { p: {} = a} = a) {        return ({} = a, [] = a, { p: {} = a } = a) => a;    }}), "() => void");
AssertType(function () {    let a: any;    let {} = a;    let {} = a;    const {} = a;    let [] = a;    let [] = a;    const [] = a;    let {} = a, [] = a;    let {} = a, [] = a;    const {} = a, [] = a;    let { p1: {}, p2: [] } = a;    let { p1: {}, p2: [] } = a;    const { p1: {}, p2: [] } = a;    for (let {} = {}, {} = {}; false; void 0) {    }    function f({} = a, [] = a, { p: {} = a} = a) {        return ({} = a, [] = a, { p: {} = a } = a) => a;    }}, "() => void");
AssertType((function () {    let a: any;    let {} = a;    let {} = a;    const {} = a;    let [] = a;    let [] = a;    const [] = a;    let {} = a, [] = a;    let {} = a, [] = a;    const {} = a, [] = a;    let { p1: {}, p2: [] } = a;    let { p1: {}, p2: [] } = a;    const { p1: {}, p2: [] } = a;    for (let {} = {}, {} = {}; false; void 0) {    }    function f({} = a, [] = a, { p: {} = a} = a) {        return ({} = a, [] = a, { p: {} = a } = a) => a;    }})(), "void");

    let a: any;
AssertType(a, "any");

    let {} = a;
AssertType(a, "any");

    let {} = a;
AssertType(a, "any");

    const {} = a;
AssertType(a, "any");

    let [] = a;
AssertType(a, "any");

    let [] = a;
AssertType(a, "any");

    const [] = a;
AssertType(a, "any");

    let {} = a, [] = a;
AssertType(a, "any");
AssertType(a, "any");

    let {} = a, [] = a;
AssertType(a, "any");
AssertType(a, "any");

    const {} = a, [] = a;
AssertType(a, "any");
AssertType(a, "any");

    let { p1: {}, p2: [] } = a;
AssertType(p1, "any");
AssertType(p2, "any");
AssertType(a, "any");

    let { p1: {}, p2: [] } = a;
AssertType(p1, "any");
AssertType(p2, "any");
AssertType(a, "any");

    const { p1: {}, p2: [] } = a;
AssertType(p1, "any");
AssertType(p2, "any");
AssertType(a, "any");

    for (let {} = {}, {} = {}; false; void 0) {
AssertType({}, "{}");
AssertType({}, "{}");
AssertType(false, "boolean");
AssertType(void 0, "undefined");
AssertType(0, "int");
    }

    function f({} = a, [] = a, { p: {} = a} = a) {
AssertType(f, "(?any, ?any, ?any) => (?any, ?any, ?any) => any");
AssertType(a, "any");
AssertType(a, "any");
AssertType(p, "any");
AssertType(a, "any");
AssertType(a, "any");

AssertType(({} = a, [] = a, { p: {} = a } = a) => a, "(?any, ?any, ?any) => any");
AssertType(a, "any");
AssertType(a, "any");
AssertType(p, "any");
AssertType(a, "any");
AssertType(a, "any");
AssertType(a, "any");
        return ({} = a, [] = a, { p: {} = a } = a) => a;
    }
})();

(function () {
AssertType((function () {    const ns: number[][] = [];    for (let {} of ns) {    }    for (let {} of ns) {    }    for (const {} of ns) {    }    for (let [] of ns) {    }    for (let [] of ns) {    }    for (const [] of ns) {    }})(), "void");
AssertType((function () {    const ns: number[][] = [];    for (let {} of ns) {    }    for (let {} of ns) {    }    for (const {} of ns) {    }    for (let [] of ns) {    }    for (let [] of ns) {    }    for (const [] of ns) {    }}), "() => void");
AssertType(function () {    const ns: number[][] = [];    for (let {} of ns) {    }    for (let {} of ns) {    }    for (const {} of ns) {    }    for (let [] of ns) {    }    for (let [] of ns) {    }    for (const [] of ns) {    }}, "() => void");

    const ns: number[][] = [];
AssertType(ns, "number[][]");
AssertType([], "undefined[]");

    for (let {} of ns) {
AssertType(ns, "number[][]");
    }

    for (let {} of ns) {
AssertType(ns, "number[][]");
    }

    for (const {} of ns) {
AssertType(ns, "number[][]");
    }

    for (let [] of ns) {
AssertType(ns, "number[][]");
    }

    for (let [] of ns) {
AssertType(ns, "number[][]");
    }

    for (const [] of ns) {
AssertType(ns, "number[][]");
    }
})();

