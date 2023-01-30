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

// === tests/cases/compiler/emitOneLineVariableDeclarationRemoveCommentsFalse.ts ===
declare function AssertType(value:any, type:string):void;
let a = /*[[${something}]]*/ {};
AssertType(a, "{}");
AssertType({}, "{}");

let b: any = /*[[${something}]]*/ {};
AssertType(b, "any");
AssertType({}, "{}");

let c: { hoge: boolean } = /*[[${something}]]*/ { hoge: true };
AssertType(c, "{ hoge: boolean; }");
AssertType(hoge, "boolean");
AssertType({ hoge: true }, "{ hoge: true; }");
AssertType(hoge, "boolean");
AssertType(true, "boolean");

let d: any  /*[[${something}]]*/ = {};
AssertType(d, "any");
AssertType({}, "{}");

let e/*[[${something}]]*/: any   = {};
AssertType(e, "any");
AssertType({}, "{}");

let f = /* comment1 */ d(e);
AssertType(f, "any");
AssertType(d(e), "any");
AssertType(d, "any");
AssertType(e, "any");

let g: any = /* comment2 */ d(e);
AssertType(g, "any");
AssertType(d(e), "any");
AssertType(d, "any");
AssertType(e, "any");


