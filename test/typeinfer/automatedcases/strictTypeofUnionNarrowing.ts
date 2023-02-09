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

// === tests/cases/compiler/strictTypeofUnionNarrowing.ts ===
declare function AssertType(value:any, type:string):void;
function stringify1(anything: { toString(): string } | undefined): string {
AssertType(typeof anything === "string" ? anything.toUpperCase() : "", "string");
AssertType(typeof anything === "string", "boolean");
AssertType(typeof anything, "union");
AssertType(anything, "union");
AssertType("string", "string");
AssertType(anything.toUpperCase(), "string");
AssertType(anything.toUpperCase, "() => string");
AssertType("", "string");
    return typeof anything === "string" ? anything.toUpperCase() : "";
}

function stringify2(anything: {} | undefined): string {
AssertType(typeof anything === "string" ? anything.toUpperCase() : "", "string");
AssertType(typeof anything === "string", "boolean");
AssertType(typeof anything, "union");
AssertType(anything, "union");
AssertType("string", "string");
AssertType(anything.toUpperCase(), "string");
AssertType(anything.toUpperCase, "() => string");
AssertType("", "string");
    return typeof anything === "string" ? anything.toUpperCase() : "";
}

function stringify3(anything: unknown | undefined): string { // should simplify to just `unknown` which should narrow fine
AssertType(typeof anything === "string" ? anything.toUpperCase() : "", "string");
AssertType(typeof anything === "string", "boolean");
AssertType(typeof anything, "union");
AssertType(anything, "unknown");
AssertType("string", "string");
AssertType(anything.toUpperCase(), "string");
AssertType(anything.toUpperCase, "() => string");
AssertType("", "string");
    return typeof anything === "string" ? anything.toUpperCase() : "";
}

function stringify4(anything: { toString?(): string } | undefined): string {
AssertType(typeof anything === "string" ? anything.toUpperCase() : "", "string");
AssertType(typeof anything === "string", "boolean");
AssertType(typeof anything, "union");
AssertType(anything, "union");
AssertType("string", "string");
AssertType(anything.toUpperCase(), "string");
AssertType(anything.toUpperCase, "() => string");
AssertType("", "string");
    return typeof anything === "string" ? anything.toUpperCase() : "";
}


