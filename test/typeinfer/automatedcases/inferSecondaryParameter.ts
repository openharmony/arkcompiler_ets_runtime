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

// === tests/cases/compiler/inferSecondaryParameter.ts ===
declare function AssertType(value:any, type:string):void;
// type inference on 'bug' should give 'any'

interface Ib { m(test: string, fn: Function); }

let b: Ib = { m: function (test: string, fn: Function) { } };
AssertType(b, "Ib");
AssertType({ m: function (test: string, fn: Function) { } }, "{ m: (string, Function) => void; }");
AssertType(m, "(string, Function) => void");
AssertType(function (test: string, fn: Function) { }, "(string, Function) => void");
AssertType(test, "string");
AssertType(fn, "Function");

b.m("test", function (bug) {
AssertType(b.m("test", function (bug) {    let a: number = bug;}), "any");
AssertType(b.m, "(string, Function) => any");
AssertType("test", "string");
AssertType(function (bug) {    let a: number = bug;}, "(any) => void");
AssertType(bug, "any");

    let a: number = bug;
AssertType(a, "number");
AssertType(bug, "any");

});

