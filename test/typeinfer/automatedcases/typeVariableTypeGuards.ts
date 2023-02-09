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

// === tests/cases/compiler/typeVariableTypeGuards.ts ===
declare function AssertType(value:any, type:string):void;
// Repro from #14091

interface Foo {
    foo(): void
}

class A<P extends Partial<Foo>> {
    constructor(public props: Readonly<P>) {}
    doSomething() {
        this.props.foo && this.props.foo()
AssertType(this.props.foo && this.props.foo(), "union");
AssertType(this.props.foo, "union");
AssertType(this.props, "Readonly<P>");
AssertType(this, "this");
AssertType(this.props.foo(), "void");
AssertType(this.props.foo, "() => void");
AssertType(this.props, "Readonly<P>");
AssertType(this, "this");
    }
}

// Repro from #14415

interface Banana {
    color: 'yellow';
}

class Monkey<T extends Banana | undefined> {
    constructor(public a: T) {}
    render() {
        if (this.a) {
AssertType(this.a, "T");
AssertType(this, "this");

            this.a.color;
AssertType(this.a.color, "string");
AssertType(this.a, "Banana");
AssertType(this, "this");
        }
    }
}

interface BigBanana extends Banana {
}

class BigMonkey extends Monkey<BigBanana> {
    render() {
        if (this.a) {
AssertType(this.a, "BigBanana");
AssertType(this, "this");

            this.a.color;
AssertType(this.a.color, "string");
AssertType(this.a, "BigBanana");
AssertType(this, "this");
        }
    }
}

// Another repro

type Item = {
    (): string;
    x: string;
}

function f1<T extends Item | undefined>(obj: T) {
    if (obj) {
AssertType(obj, "T");

        obj.x;
AssertType(obj.x, "string");

        obj["x"];
AssertType(obj["x"], "string");
AssertType(obj, "Item");
AssertType("x", "string");

        obj();
AssertType(obj(), "string");
AssertType(obj, "Item");
    }
}

function f2<T extends Item | undefined>(obj: T | undefined) {
    if (obj) {
AssertType(obj, "union");

        obj.x;
AssertType(obj.x, "string");

        obj["x"];
AssertType(obj["x"], "string");
AssertType(obj, "Item");
AssertType("x", "string");

        obj();
AssertType(obj(), "string");
AssertType(obj, "Item");
    }
}

function f3<T extends Item | undefined>(obj: T | null) {
    if (obj) {
AssertType(obj, "union");

        obj.x;
AssertType(obj.x, "string");

        obj["x"];
AssertType(obj["x"], "string");
AssertType(obj, "Item");
AssertType("x", "string");

        obj();
AssertType(obj(), "string");
AssertType(obj, "Item");
    }
}

function f4<T extends string[] | undefined>(obj: T | undefined, x: number) {
    if (obj) {
AssertType(obj, "union");

        obj[x].length;
AssertType(obj[x].length, "number");
AssertType(obj[x], "string");
AssertType(obj, "string[]");
AssertType(x, "number");
    }
}

function f5<T, K extends keyof T>(obj: T | undefined, key: K) {
    if (obj) {
AssertType(obj, "union");

        obj[key];
AssertType(obj[key], "NonNullable<T>[K]");
AssertType(obj, "NonNullable<T>");
AssertType(key, "K");
    }
}


