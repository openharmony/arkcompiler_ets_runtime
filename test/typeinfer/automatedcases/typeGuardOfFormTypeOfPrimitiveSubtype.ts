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

// === tests/cases/conformance/expressions/typeGuards/typeGuardOfFormTypeOfPrimitiveSubtype.ts ===
declare function AssertType(value:any, type:string):void;
let a: {};
AssertType(a, "{}");

let b: {toString(): string};
AssertType(b, "{ toString(): string; }");
AssertType(toString, "() => string");

if (typeof a === "number") {
    let c: number = a;
AssertType(c, "number");
AssertType(a, "number");
}
if (typeof a === "string") {
    let c: string = a;
AssertType(c, "string");
AssertType(a, "string");
}
if (typeof a === "boolean") {
    let c: boolean = a;
AssertType(c, "boolean");
AssertType(a, "boolean");
}

if (typeof b === "number") {
    let c: number = b;
AssertType(c, "number");
AssertType(b, "number");
}
if (typeof b === "string") {
    let c: string = b;
AssertType(c, "string");
AssertType(b, "string");
}
if (typeof b === "boolean") {
    let c: boolean = b;
AssertType(c, "boolean");
AssertType(b, "boolean");
}


