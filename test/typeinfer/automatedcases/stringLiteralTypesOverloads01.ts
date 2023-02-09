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

// === tests/cases/conformance/types/stringLiteral/stringLiteralTypesOverloads01.ts ===
declare function AssertType(value:any, type:string):void;
type PrimitiveName = 'string' | 'number' | 'boolean';

function getFalsyPrimitive(x: "string"): string;
function getFalsyPrimitive(x: "number"): number;
function getFalsyPrimitive(x: "boolean"): boolean;
function getFalsyPrimitive(x: "boolean" | "string"): boolean | string;
function getFalsyPrimitive(x: "boolean" | "number"): boolean | number;
function getFalsyPrimitive(x: "number" | "string"): number | string;
function getFalsyPrimitive(x: "number" | "string" | "boolean"): number | string | boolean;
function getFalsyPrimitive(x: PrimitiveName): number | string | boolean {
    if (x === "string") {
AssertType(x === "string", "boolean");
AssertType(x, "PrimitiveName");
AssertType("string", "string");

AssertType("", "string");
        return "";
    }
    if (x === "number") {
AssertType(x === "number", "boolean");
AssertType(x, "union");
AssertType("number", "string");

AssertType(0, "int");
        return 0;
    }
    if (x === "boolean") {
AssertType(x === "boolean", "boolean");
AssertType(x, "string");
AssertType("boolean", "string");

AssertType(false, "boolean");
        return false;
    }

    // Should be unreachable.
    throw "Invalid value";
AssertType("Invalid value", "string");
}

namespace Consts1 {
    const EMPTY_STRING = getFalsyPrimitive("string");
    const ZERO = getFalsyPrimitive('number');
    const FALSE = getFalsyPrimitive("boolean");
}

const string: "string" = "string"
AssertType(string, "string");
AssertType("string", "string");

const number: "number" = "number"
AssertType(number, "string");
AssertType("number", "string");

const boolean: "boolean" = "boolean"
AssertType(boolean, "string");
AssertType("boolean", "string");

const stringOrNumber = string || number;
AssertType(stringOrNumber, "union");
AssertType(string || number, "union");
AssertType(string, "string");
AssertType(number, "string");

const stringOrBoolean = string || boolean;
AssertType(stringOrBoolean, "union");
AssertType(string || boolean, "union");
AssertType(string, "string");
AssertType(boolean, "string");

const booleanOrNumber = number || boolean;
AssertType(booleanOrNumber, "union");
AssertType(number || boolean, "union");
AssertType(number, "string");
AssertType(boolean, "string");

const stringOrBooleanOrNumber = stringOrBoolean || number;
AssertType(stringOrBooleanOrNumber, "union");
AssertType(stringOrBoolean || number, "union");
AssertType(stringOrBoolean, "union");
AssertType(number, "string");

namespace Consts2 {
    const EMPTY_STRING = getFalsyPrimitive(string);
    const ZERO = getFalsyPrimitive(number);
    const FALSE = getFalsyPrimitive(boolean);

    const a = getFalsyPrimitive(stringOrNumber);
    const b = getFalsyPrimitive(stringOrBoolean);
    const c = getFalsyPrimitive(booleanOrNumber);
    const d = getFalsyPrimitive(stringOrBooleanOrNumber);
}




