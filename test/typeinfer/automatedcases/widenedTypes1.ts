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

// === tests/cases/compiler/widenedTypes1.ts ===
declare function AssertType(value:any, type:string):void;
let a = null;
AssertType(a, "any");
AssertType(null, "null");

let b = undefined;
AssertType(b, "any");
AssertType(undefined, "undefined");

let c = {x: null};
AssertType(c, "{ x: any; }");
AssertType({x: null}, "{ x: null; }");
AssertType(x, "null");
AssertType(null, "null");

let d = [{x: null}];
AssertType(d, "{ x: any; }[]");
AssertType([{x: null}], "{ x: null; }[]");
AssertType({x: null}, "{ x: null; }");
AssertType(x, "null");
AssertType(null, "null");

let f = [null, null];
AssertType(f, "any[]");
AssertType([null, null], "null[]");
AssertType(null, "null");
AssertType(null, "null");

let g = [undefined, undefined];
AssertType(g, "any[]");
AssertType([undefined, undefined], "undefined[]");
AssertType(undefined, "undefined");
AssertType(undefined, "undefined");

let h = [{x: undefined}];
AssertType(h, "{ x: any; }[]");
AssertType([{x: undefined}], "{ x: undefined; }[]");
AssertType({x: undefined}, "{ x: undefined; }");
AssertType(x, "undefined");
AssertType(undefined, "undefined");


