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

// === tests/cases/conformance/types/stringLiteral/stringLiteralTypesTypePredicates01.ts ===
declare function AssertType(value:any, type:string):void;
type Kind = "A" | "B"

function kindIs(kind: Kind, is: "A"): kind is "A";
function kindIs(kind: Kind, is: "B"): kind is "B";
function kindIs(kind: Kind, is: Kind): boolean {
AssertType(kind === is, "boolean");
AssertType(kind, "Kind");
AssertType(is, "Kind");
    return kind === is;
}

let x: Kind = undefined;
AssertType(x, "Kind");
AssertType(undefined, "undefined");

if (kindIs(x, "A")) {
    let a = x;
AssertType(a, "string");
AssertType(x, "string");
}
else {
    let b = x;
AssertType(b, "string");
AssertType(x, "string");
}

if (!kindIs(x, "B")) {
    let c = x;
AssertType(c, "string");
AssertType(x, "string");
}
else {
    let d = x;
AssertType(d, "string");
AssertType(x, "string");
}

