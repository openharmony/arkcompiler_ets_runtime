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

// === tests/cases/compiler/discriminantsAndPrimitives.ts ===
declare function AssertType(value:any, type:string):void;
// Repro from #10257 plus other tests

interface Foo {
    kind: "foo";
    name: string;
}

interface Bar {
    kind: "bar";
    length: string;
}

function f1(x: Foo | Bar | string) {
    if (typeof x !== 'string') {
AssertType(typeof x !== 'string', "boolean");
AssertType(typeof x, "union");
AssertType(x, "union");
AssertType('string', "string");

        switch(x.kind) {
AssertType(x.kind, "union");

            case 'foo':
AssertType('foo', "string");

                x.name;
AssertType(x.name, "string");
        }
    }
}

function f2(x: Foo | Bar | string | undefined) {
    if (typeof x === "object") {
AssertType(typeof x === "object", "boolean");
AssertType(typeof x, "union");
AssertType(x, "union");
AssertType("object", "string");

        switch(x.kind) {
AssertType(x.kind, "union");

            case 'foo':
AssertType('foo', "string");

                x.name;
AssertType(x.name, "string");
        }
    }
}

function f3(x: Foo | Bar | string | null) {
    if (x && typeof x !== "string") {
AssertType(x && typeof x !== "string", "union");
AssertType(x, "union");
AssertType(typeof x !== "string", "boolean");
AssertType(typeof x, "union");
AssertType(x, "union");
AssertType("string", "string");

        switch(x.kind) {
AssertType(x.kind, "union");

            case 'foo':
AssertType('foo', "string");

                x.name;
AssertType(x.name, "string");
        }
    }
}

function f4(x: Foo | Bar | string | number | null) {
    if (x && typeof x === "object") {
AssertType(x && typeof x === "object", "union");
AssertType(x, "union");
AssertType(typeof x === "object", "boolean");
AssertType(typeof x, "union");
AssertType(x, "union");
AssertType("object", "string");

        switch(x.kind) {
AssertType(x.kind, "union");

            case 'foo':
AssertType('foo', "string");

                x.name;
AssertType(x.name, "string");
        }
    }
}

// Repro from #31319

const enum EnumTypeNode {
    Pattern = "Pattern",
    Disjunction = "Disjunction",
}

type NodeA = Disjunction | Pattern;

interface NodeBase {
    type: NodeA["type"]
}

interface Disjunction extends NodeBase {
    type: EnumTypeNode.Disjunction
    alternatives: string[]
}

interface Pattern extends NodeBase {
    type: EnumTypeNode.Pattern
    elements: string[]
}

let n!: NodeA
AssertType(n, "NodeA");

if (n.type === "Disjunction") {
    n.alternatives.slice()
AssertType(n.alternatives.slice(), "string[]");
AssertType(n.alternatives.slice, "(?union, ?union) => string[]");
AssertType(n.alternatives, "string[]");
}
else {
    n.elements.slice() // n should be narrowed to Pattern
AssertType(n.elements.slice(), "string[]");
AssertType(n.elements.slice, "(?union, ?union) => string[]");
AssertType(n.elements, "string[]");
}


