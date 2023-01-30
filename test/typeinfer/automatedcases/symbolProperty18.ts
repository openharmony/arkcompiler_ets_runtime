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

// === tests/cases/conformance/es6/Symbols/symbolProperty18.ts ===
declare function AssertType(value:any, type:string):void;
let i = {
AssertType(i, "{ [Symbol.iterator]: number; [Symbol.toStringTag](): string; [Symbol.toPrimitive]: boolean; }");
AssertType({    [Symbol.iterator]: 0,    [Symbol.toStringTag]() { return "" },    set [Symbol.toPrimitive](p: boolean) { }}, "{ [Symbol.iterator]: number; [Symbol.toStringTag](): string; [Symbol.toPrimitive]: boolean; }");

    [Symbol.iterator]: 0,
AssertType([Symbol.iterator], "number");
AssertType(Symbol.iterator, "unique symbol");
AssertType(0, "int");

    [Symbol.toStringTag]() { 
AssertType([Symbol.toStringTag], "() => string");
AssertType(Symbol.toStringTag, "unique symbol");
AssertType("", "string");
return "" },

    set [Symbol.toPrimitive](p: boolean) { 
AssertType([Symbol.toPrimitive], "boolean");

AssertType(Symbol.toPrimitive, "unique symbol");

AssertType(p, "boolean");
}
}

let it = i[Symbol.iterator];
AssertType(it, "number");
AssertType(i[Symbol.iterator], "number");
AssertType(i, "{ [Symbol.iterator]: number; [Symbol.toStringTag](): string; [Symbol.toPrimitive]: boolean; }");
AssertType(Symbol.iterator, "unique symbol");

let str = i[Symbol.toStringTag]();
AssertType(str, "string");
AssertType(i[Symbol.toStringTag](), "string");
AssertType(i[Symbol.toStringTag], "() => string");
AssertType(i, "{ [Symbol.iterator]: number; [Symbol.toStringTag](): string; [Symbol.toPrimitive]: boolean; }");
AssertType(Symbol.toStringTag, "unique symbol");

i[Symbol.toPrimitive] = false;
AssertType(i[Symbol.toPrimitive] = false, "boolean");
AssertType(i[Symbol.toPrimitive], "boolean");
AssertType(i, "{ [Symbol.iterator]: number; [Symbol.toStringTag](): string; [Symbol.toPrimitive]: boolean; }");
AssertType(Symbol.toPrimitive, "unique symbol");
AssertType(false, "boolean");


