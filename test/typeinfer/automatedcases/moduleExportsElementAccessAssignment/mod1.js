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

// === tests/cases/conformance/jsdoc/mod1.js ===
declare function AssertType(value:any, type:string):void;
exports.a = { x: "x" };
AssertType(exports.a = { x: "x" }, "{ x: string; }");
AssertType(exports.a, "{ x: string; }");
AssertType({ x: "x" }, "{ x: string; }");
AssertType(x, "string");
AssertType("x", "string");

exports["b"] = { x: "x" };
AssertType(exports["b"] = { x: "x" }, "{ x: string; }");
AssertType(exports["b"], "{ x: string; }");
AssertType(exports, "typeof import("tests/cases/conformance/jsdoc/mod1")");
AssertType("b", "string");
AssertType({ x: "x" }, "{ x: string; }");
AssertType(x, "string");
AssertType("x", "string");

exports["default"] = { x: "x" };
AssertType(exports["default"] = { x: "x" }, "{ x: string; }");
AssertType(exports["default"], "{ x: string; }");
AssertType(exports, "typeof import("tests/cases/conformance/jsdoc/mod1")");
AssertType("default", "string");
AssertType({ x: "x" }, "{ x: string; }");
AssertType(x, "string");
AssertType("x", "string");

module.exports["c"] = { x: "x" };
AssertType(module.exports["c"] = { x: "x" }, "{ x: string; }");
AssertType(module.exports["c"], "{ x: string; }");
AssertType(module.exports, "typeof module.exports");
AssertType("c", "string");
AssertType({ x: "x" }, "{ x: string; }");
AssertType(x, "string");
AssertType("x", "string");

module["exports"]["d"] = {};
AssertType(module["exports"]["d"] = {}, "typeof "d"");
AssertType(module["exports"]["d"], "typeof "d"");
AssertType(module["exports"], "typeof module.exports");
AssertType(module, "{ exports: typeof module.exports; }");
AssertType("exports", "string");
AssertType("d", "string");
AssertType({}, "{}");

module["exports"]["d"].e = 0;
AssertType(module["exports"]["d"].e = 0, "int");
AssertType(module["exports"]["d"].e, "number");
AssertType(0, "int");


