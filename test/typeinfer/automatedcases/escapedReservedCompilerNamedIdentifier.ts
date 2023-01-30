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

// === tests/cases/compiler/escapedReservedCompilerNamedIdentifier.ts ===
declare function AssertType(value:any, type:string):void;
// double underscores
let __proto__ = 10;
AssertType(__proto__, "number");
AssertType(10, "int");

let o = {
AssertType(o, "{ __proto__: number; }");
AssertType({    "__proto__": 0}, "{ __proto__: number; }");

    "__proto__": 0
AssertType("__proto__", "number");
AssertType(0, "int");

};
let b = o["__proto__"];
AssertType(b, "number");
AssertType(o["__proto__"], "number");
AssertType(o, "{ __proto__: number; }");
AssertType("__proto__", "string");

let o1 = {
AssertType(o1, "{ __proto__: number; }");
AssertType({    __proto__: 0}, "{ __proto__: number; }");

    __proto__: 0
AssertType(__proto__, "number");
AssertType(0, "int");

};
let b1 = o1["__proto__"];
AssertType(b1, "number");
AssertType(o1["__proto__"], "number");
AssertType(o1, "{ __proto__: number; }");
AssertType("__proto__", "string");

// Triple underscores
let ___proto__ = 10;
AssertType(___proto__, "number");
AssertType(10, "int");

let o2 = {
AssertType(o2, "{ ___proto__: number; }");
AssertType({    "___proto__": 0}, "{ ___proto__: number; }");

    "___proto__": 0
AssertType("___proto__", "number");
AssertType(0, "int");

};
let b2 = o2["___proto__"];
AssertType(b2, "number");
AssertType(o2["___proto__"], "number");
AssertType(o2, "{ ___proto__: number; }");
AssertType("___proto__", "string");

let o3 = {
AssertType(o3, "{ ___proto__: number; }");
AssertType({    ___proto__: 0}, "{ ___proto__: number; }");

    ___proto__: 0
AssertType(___proto__, "number");
AssertType(0, "int");

};
let b3 = o3["___proto__"];
AssertType(b3, "number");
AssertType(o3["___proto__"], "number");
AssertType(o3, "{ ___proto__: number; }");
AssertType("___proto__", "string");

// One underscore
let _proto__ = 10;
AssertType(_proto__, "number");
AssertType(10, "int");

let o4 = {
AssertType(o4, "{ _proto__: number; }");
AssertType({    "_proto__": 0}, "{ _proto__: number; }");

    "_proto__": 0
AssertType("_proto__", "number");
AssertType(0, "int");

};
let b4 = o4["_proto__"];
AssertType(b4, "number");
AssertType(o4["_proto__"], "number");
AssertType(o4, "{ _proto__: number; }");
AssertType("_proto__", "string");

let o5 = {
AssertType(o5, "{ _proto__: number; }");
AssertType({    _proto__: 0}, "{ _proto__: number; }");

    _proto__: 0
AssertType(_proto__, "number");
AssertType(0, "int");

};
let b5 = o5["_proto__"];
AssertType(b5, "number");
AssertType(o5["_proto__"], "number");
AssertType(o5, "{ _proto__: number; }");
AssertType("_proto__", "string");


