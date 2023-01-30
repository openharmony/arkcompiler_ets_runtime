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

// === tests/cases/conformance/types/stringLiteral/stringLiteralTypesAsTags02.ts ===
declare function AssertType(value:any, type:string):void;
type Kind = "A" | "B"

interface Entity {
    kind: Kind;
}

interface A extends Entity {
    kind: "A";
    a: number;
}

interface B extends Entity {
    kind: "B";
    b: string;
}

function hasKind(entity: Entity, kind: "A"): entity is A;
function hasKind(entity: Entity, kind: "B"): entity is B;
function hasKind(entity: Entity, kind: Kind): entity is (A | B) {
AssertType(entity.kind === kind, "boolean");
AssertType(entity.kind, "Kind");
AssertType(kind, "Kind");
    return entity.kind === kind;
}

let x: A = {
AssertType(x, "A");
AssertType({    kind: "A",    a: 100,}, "{ kind: "A"; a: number; }");

    kind: "A",
AssertType(kind, "string");
AssertType("A", "string");

    a: 100,
AssertType(a, "number");
AssertType(100, "int");
}

if (hasKind(x, "A")) {
    let a = x;
AssertType(a, "A");
AssertType(x, "A");
}
else {
    let b = x;
AssertType(b, "never");
AssertType(x, "never");
}

if (!hasKind(x, "B")) {
    let c = x;
AssertType(c, "A");
AssertType(x, "A");
}
else {
    let d = x;
AssertType(d, "never");
AssertType(x, "never");
}

