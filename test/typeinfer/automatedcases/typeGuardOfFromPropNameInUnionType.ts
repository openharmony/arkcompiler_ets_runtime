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

// === tests/cases/conformance/expressions/typeGuards/typeGuardOfFromPropNameInUnionType.ts ===
declare function AssertType(value:any, type:string):void;
class A { a: string; }
class B { b: number; }
class C { b: Object; }
class D { a: Date; }

function namedClasses(x: A | B) {
    if ("a" in x) {
AssertType("a" in x, "boolean");
AssertType("a", "string");
AssertType(x, "union");

        x.a = "1";
AssertType(x.a = "1", "string");
AssertType(x.a, "string");
AssertType("1", "string");

    } else {
        x.b = 1;
AssertType(x.b = 1, "int");
AssertType(x.b, "number");
AssertType(1, "int");
    }
}

function multipleClasses(x: A | B | C | D) {
    if ("a" in x) {
AssertType("a" in x, "boolean");
AssertType("a", "string");
AssertType(x, "union");

        let y: string | Date = x.a;
AssertType(y, "union");
AssertType(x.a, "union");

    } else {
        let z: number | Object = x.b;
AssertType(z, "union");
AssertType(x.b, "union");
    }
}

function anonymousClasses(x: { a: string; } | { b: number; }) {
    if ("a" in x) {
AssertType("a" in x, "boolean");
AssertType("a", "string");
AssertType(x, "union");

        let y: string = x.a;
AssertType(y, "string");
AssertType(x.a, "string");

    } else {
        let z: number = x.b;
AssertType(z, "number");
AssertType(x.b, "number");
    }
}

class AWithOptionalProp { a?: string; }
class BWithOptionalProp { b?: string; }

function positiveTestClassesWithOptionalProperties(x: AWithOptionalProp | BWithOptionalProp) {
    if ("a" in x) {
AssertType("a" in x, "boolean");
AssertType("a", "string");
AssertType(x, "union");

        x.a = "1";
AssertType(x.a = "1", "string");
AssertType(x.a, "string");
AssertType("1", "string");

    } else {
        const y: string = x instanceof AWithOptionalProp
AssertType(y, "string");
AssertType(x instanceof AWithOptionalProp            ? x.a            : x.b, "string");
AssertType(x instanceof AWithOptionalProp, "boolean");
AssertType(x, "union");
AssertType(AWithOptionalProp, "typeof AWithOptionalProp");

            ? x.a
AssertType(x.a, "string");

            : x.b
AssertType(x.b, "string");
    }
}

function inParenthesizedExpression(x: A | B) {
    if ("a" in (x)) {
AssertType("a" in (x), "boolean");
AssertType("a", "string");
AssertType((x), "union");
AssertType(x, "union");

        let y: string = x.a;
AssertType(y, "string");
AssertType(x.a, "string");

    } else {
        let z: number = x.b;
AssertType(z, "number");
AssertType(x.b, "number");
    }
}

class ClassWithUnionProp { prop: A | B; }

function inProperty(x: ClassWithUnionProp) {
    if ("a" in x.prop) {
AssertType("a" in x.prop, "boolean");
AssertType("a", "string");
AssertType(x.prop, "union");

        let y: string = x.prop.a;
AssertType(y, "string");
AssertType(x.prop.a, "string");
AssertType(x.prop, "A");

    } else {
        let z: number = x.prop.b;
AssertType(z, "number");
AssertType(x.prop.b, "number");
AssertType(x.prop, "B");
    }
}

class NestedClassWithProp { outer: ClassWithUnionProp; }

function innestedProperty(x: NestedClassWithProp) {
    if ("a" in x.outer.prop) {
AssertType("a" in x.outer.prop, "boolean");
AssertType("a", "string");
AssertType(x.outer.prop, "union");
AssertType(x.outer, "ClassWithUnionProp");

        let y: string = x.outer.prop.a;
AssertType(y, "string");
AssertType(x.outer.prop.a, "string");
AssertType(x.outer.prop, "A");
AssertType(x.outer, "ClassWithUnionProp");

    } else {
        let z: number = x.outer.prop.b;
AssertType(z, "number");
AssertType(x.outer.prop.b, "number");
AssertType(x.outer.prop, "B");
AssertType(x.outer, "ClassWithUnionProp");
    }
}

class InMemberOfClass {
    protected prop: A | B;
    inThis() {
        if ("a" in this.prop) {
AssertType("a" in this.prop, "boolean");
AssertType("a", "string");
AssertType(this.prop, "union");
AssertType(this, "this");

            let y: string = this.prop.a;
AssertType(y, "string");
AssertType(this.prop.a, "string");
AssertType(this.prop, "A");
AssertType(this, "this");

        } else {
            let z: number = this.prop.b;
AssertType(z, "number");
AssertType(this.prop.b, "number");
AssertType(this.prop, "B");
AssertType(this, "this");
        }
    }
}

// added for completeness
class SelfAssert {
    a: string;
    inThis() {
        if ("a" in this) {
AssertType("a" in this, "boolean");
AssertType("a", "string");
AssertType(this, "this");

            let y: string = this.a;
AssertType(y, "string");
AssertType(this.a, "string");
AssertType(this, "this");

        } else {
        }
    }
}

interface Indexed {
    [s: string]: any;
}

function f(i: Indexed) {
    if ("a" in i) {
AssertType("a" in i, "boolean");
AssertType("a", "string");
AssertType(i, "Indexed");

AssertType(i.a, "any");
        return i.a;
    }
    else if ("b" in i) {
AssertType("b" in i, "boolean");
AssertType("b", "string");
AssertType(i, "Indexed");

AssertType(i.b, "any");
        return i.b;
    }
AssertType("c" in i && i.c, "any");
AssertType("c" in i, "boolean");
AssertType("c", "string");
AssertType(i, "Indexed");
AssertType(i.c, "any");
    return "c" in i && i.c;
}


