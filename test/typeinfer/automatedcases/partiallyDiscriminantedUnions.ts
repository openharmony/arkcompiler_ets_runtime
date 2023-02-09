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

// === tests/cases/compiler/partiallyDiscriminantedUnions.ts ===
declare function AssertType(value:any, type:string):void;
// Repro from #10586

interface A1 {
    type: 'a';
    subtype: 1;
}

interface A2 {
    type: 'a';
    subtype: 2;
    foo: number;
}

interface B {
    type: 'b';
}

type AB = A1 | A2 | B;

const ab: AB = <AB>{};
AssertType(ab, "AB");
AssertType(<AB>{}, "AB");
AssertType({}, "{}");

if (ab.type === 'a') {
    if (ab.subtype === 2) {
AssertType(ab.subtype === 2, "boolean");
AssertType(ab.subtype, "union");
AssertType(2, "int");

        ab.foo;
AssertType(ab.foo, "number");
    }
}

// Repro from #11185

class Square { kind: "square"; }
class Circle { kind: "circle"; }

type Shape = Circle | Square;
type Shapes = Shape | Array<Shape>;

function isShape(s : Shapes): s is Shape {
AssertType(!Array.isArray(s), "boolean");
AssertType(Array.isArray(s), "boolean");
AssertType(Array.isArray, "(any) => arg is any[]");
AssertType(s, "Shapes");
    return !Array.isArray(s);
}

function fail(s: Shapes) {
    if (isShape(s)) {
AssertType(isShape(s), "boolean");
AssertType(isShape, "(Shapes) => s is Shape");
AssertType(s, "Shapes");

        if (s.kind === "circle") {
AssertType(s.kind === "circle", "boolean");
AssertType(s.kind, "union");
AssertType("circle", "string");

            let c: Circle = s;
AssertType(c, "Circle");
AssertType(s, "Circle");
        }
    }
}

