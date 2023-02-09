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

// === tests/cases/conformance/classes/mixinClassesAnnotated.ts ===
declare function AssertType(value:any, type:string):void;
type Constructor<T> = new(...args: any[]) => T;

class Base {
    constructor(public x: number, public y: number) {}
}

class Derived extends Base {
    constructor(x: number, y: number, public z: number) {
        super(x, y);
AssertType(super(x, y), "void");
AssertType(super, "typeof Base");
AssertType(x, "number");
AssertType(y, "number");
    }
}

interface Printable {
    print(): void;
}

const Printable = <T extends Constructor<Base>>(superClass: T): Constructor<Printable> & { message: string } & T =>
AssertType(Printable, "<T extends Constructor<Base>>(T) => Constructor<Printable> & { message: string; } & T");
AssertType(<T extends Constructor<Base>>(superClass: T): Constructor<Printable> & { message: string } & T =>    class extends superClass {        static message = "hello";        print() {            const output = this.x + "," + this.y;        }    }, "<T extends Constructor<Base>>(T) => Constructor<Printable> & { message: string; } & T");
AssertType(superClass, "T");
AssertType(message, "string");

    class extends superClass {
AssertType(class extends superClass {        static message = "hello";        print() {            const output = this.x + "," + this.y;        }    }, "{ new (...any[]): (Anonymous class); prototype: Printable<any>.(Anonymous class); message: string; } & T");

        static message = "hello";
AssertType(message, "string");
AssertType("hello", "string");

        print() {
AssertType(print, "() => void");

            const output = this.x + "," + this.y;
AssertType(output, "string");
AssertType(this.x + "," + this.y, "string");
AssertType(this.x + ",", "string");
AssertType(this.x, "number");
AssertType(this, "this");
AssertType(",", "string");
AssertType(this.y, "number");
AssertType(this, "this");
        }
    }

interface Tagged {
    _tag: string;
}

function Tagged<T extends Constructor<{}>>(superClass: T): Constructor<Tagged> & T {
    class C extends superClass {
        _tag: string;
        constructor(...args: any[]) {
            super(...args);
AssertType(super(...args), "void");
AssertType(super, "T");
AssertType(...args, "any");
AssertType(args, "any[]");

            this._tag = "hello";
AssertType(this._tag = "hello", "string");
AssertType(this._tag, "string");
AssertType(this, "this");
AssertType("hello", "string");
        }
    }
AssertType(C, "{ new (...any[]): C; prototype: Tagged<any>.C; } & T");
    return C;
}

const Thing1 = Tagged(Derived);
AssertType(Thing1, "Constructor<Tagged> & typeof Derived");
AssertType(Tagged(Derived), "Constructor<Tagged> & typeof Derived");
AssertType(Tagged, "<T extends Constructor<{}>>(T) => Constructor<Tagged> & T");
AssertType(Derived, "typeof Derived");

const Thing2 = Tagged(Printable(Derived));
AssertType(Thing2, "Constructor<Tagged> & Constructor<Printable> & { message: string; } & typeof Derived");
AssertType(Tagged(Printable(Derived)), "Constructor<Tagged> & Constructor<Printable> & { message: string; } & typeof Derived");
AssertType(Tagged, "<T extends Constructor<{}>>(T) => Constructor<Tagged> & T");
AssertType(Printable(Derived), "Constructor<Printable> & { message: string; } & typeof Derived");
AssertType(Printable, "<T extends Constructor<Base>>(T) => Constructor<Printable> & { message: string; } & T");
AssertType(Derived, "typeof Derived");

Thing2.message;
AssertType(Thing2.message, "string");

function f1() {
    const thing = new Thing1(1, 2, 3);
AssertType(thing, "Tagged & Derived");
AssertType(new Thing1(1, 2, 3), "Tagged & Derived");
AssertType(Thing1, "Constructor<Tagged> & typeof Derived");
AssertType(1, "int");
AssertType(2, "int");
AssertType(3, "int");

    thing.x;
AssertType(thing.x, "number");

    thing._tag;
AssertType(thing._tag, "string");
}

function f2() {
    const thing = new Thing2(1, 2, 3);
AssertType(thing, "Tagged & Printable & Derived");
AssertType(new Thing2(1, 2, 3), "Tagged & Printable & Derived");
AssertType(Thing2, "Constructor<Tagged> & Constructor<Printable> & { message: string; } & typeof Derived");
AssertType(1, "int");
AssertType(2, "int");
AssertType(3, "int");

    thing.x;
AssertType(thing.x, "number");

    thing._tag;
AssertType(thing._tag, "string");

    thing.print();
AssertType(thing.print(), "void");
AssertType(thing.print, "() => void");
}

class Thing3 extends Thing2 {
    constructor(tag: string) {
        super(10, 20, 30);
AssertType(super(10, 20, 30), "void");
AssertType(super, "Constructor<Tagged> & Constructor<Printable> & { message: string; } & typeof Derived");
AssertType(10, "int");
AssertType(20, "int");
AssertType(30, "int");

        this._tag = tag;
AssertType(this._tag = tag, "string");
AssertType(this._tag, "string");
AssertType(this, "this");
AssertType(tag, "string");
    }
    test() {
        this.print();
AssertType(this.print(), "void");
AssertType(this.print, "() => void");
AssertType(this, "this");
    }
}


