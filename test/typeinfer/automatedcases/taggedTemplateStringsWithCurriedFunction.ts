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

// === tests/cases/compiler/taggedTemplateStringsWithCurriedFunction.ts ===
declare function AssertType(value:any, type:string):void;
// Originated from #38558

const f = _ => (..._) => "";
AssertType(f, "(any) => (...any[]) => string");
AssertType(_ => (..._) => "", "(any) => (...any[]) => string");
AssertType(_, "any");
AssertType((..._) => "", "(...any[]) => string");
AssertType(_, "any[]");
AssertType("", "string");

f({ ...{ x: 0 } })``;
AssertType(f({ ...{ x: 0 } })``, "string");
AssertType(f({ ...{ x: 0 } }), "(...any[]) => string");
AssertType(f, "(any) => (...any[]) => string");
AssertType({ ...{ x: 0 } }, "{ x: number; }");
AssertType({ x: 0 }, "{ x: number; }");
AssertType(x, "number");
AssertType(0, "int");
AssertType(``, "string");

f({ ...{ x: 0 } })`x`;
AssertType(f({ ...{ x: 0 } })`x`, "string");
AssertType(f({ ...{ x: 0 } }), "(...any[]) => string");
AssertType(f, "(any) => (...any[]) => string");
AssertType({ ...{ x: 0 } }, "{ x: number; }");
AssertType({ x: 0 }, "{ x: number; }");
AssertType(x, "number");
AssertType(0, "int");
AssertType(`x`, "string");

f({ ...{ x: 0 } })`x${f}x`;
AssertType(f({ ...{ x: 0 } })`x${f}x`, "string");
AssertType(f({ ...{ x: 0 } }), "(...any[]) => string");
AssertType(f, "(any) => (...any[]) => string");
AssertType({ ...{ x: 0 } }, "{ x: number; }");
AssertType({ x: 0 }, "{ x: number; }");
AssertType(x, "number");
AssertType(0, "int");
AssertType(`x${f}x`, "string");
AssertType(f, "(any) => (...any[]) => string");

f({ ...{ x: 0 }, y: (() => 1)() })``;
AssertType(f({ ...{ x: 0 }, y: (() => 1)() })``, "string");
AssertType(f({ ...{ x: 0 }, y: (() => 1)() }), "(...any[]) => string");
AssertType(f, "(any) => (...any[]) => string");
AssertType({ ...{ x: 0 }, y: (() => 1)() }, "{ y: number; x: number; }");
AssertType({ x: 0 }, "{ x: number; }");
AssertType(x, "number");
AssertType(0, "int");
AssertType(y, "number");
AssertType((() => 1)(), "number");
AssertType((() => 1), "() => number");
AssertType(() => 1, "() => number");
AssertType(1, "int");
AssertType(``, "string");

f({ x: (() => 1)(), ...{ y: 1 } })``;
AssertType(f({ x: (() => 1)(), ...{ y: 1 } })``, "string");
AssertType(f({ x: (() => 1)(), ...{ y: 1 } }), "(...any[]) => string");
AssertType(f, "(any) => (...any[]) => string");
AssertType({ x: (() => 1)(), ...{ y: 1 } }, "{ y: number; x: number; }");
AssertType(x, "number");
AssertType((() => 1)(), "number");
AssertType((() => 1), "() => number");
AssertType(() => 1, "() => number");
AssertType(1, "int");
AssertType({ y: 1 }, "{ y: number; }");
AssertType(y, "number");
AssertType(1, "int");
AssertType(``, "string");


