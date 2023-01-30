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

// === tests/cases/conformance/classes/mixinClassesAnonymous.ts ===
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

const Printable = <T extends Constructor<Base>>(superClass: T) => class extends superClass {
AssertType(Printable, "<T extends Constructor<Base>>(T) => { new (...any[]): (Anonymous class); prototype: Printable<any>.(Anonymous class); message: string; } & T");
AssertType(<T extends Constructor<Base>>(superClass: T) => class extends superClass {    static message = "hello";    print() {        const output = this.x + "," + this.y;    }}, "<T extends Constructor<Base>>(T) => { new (...any[]): (Anonymous class); prototype: Printable<any>.(Anonymous class); message: string; } & T");
AssertType(superClass, "T");
AssertType(class extends superClass {    static message = "hello";    print() {        const output = this.x + "," + this.y;    }}, "{ new (...any[]): (Anonymous class); prototype: Printable<any>.(Anonymous class); message: string; } & T");

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

function Tagged<T extends Constructor<{}>>(superClass: T) {
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
AssertType(Thing1, "{ new (...any[]): Tagged<typeof Derived>.C; prototype: Tagged<any>.C; } & typeof Derived");
AssertType(Tagged(Derived), "{ new (...any[]): Tagged<typeof Derived>.C; prototype: Tagged<any>.C; } & typeof Derived");
AssertType(Tagged, "<T extends Constructor<{}>>(T) => { new (...any[]): C; prototype: Tagged<any>.C; } & T");
AssertType(Derived, "typeof Derived");

const Thing2 = Tagged(Printable(Derived));
AssertType(Thing2, "{ new (...any[]): Tagged<{ new (...any[]): Printable<typeof Derived>.(Anonymous class); prototype: Printable<any>.(Anonymous class); message: string; } & typeof Derived>.C; prototype: Tagged<any>.C; } & { new (...any[]): Printable<typeof Derived>.(Anonymous class); prototype: Printable<any>.(Anonymous class); message: string; } & typeof Derived");
AssertType(Tagged(Printable(Derived)), "{ new (...any[]): Tagged<{ new (...any[]): Printable<typeof Derived>.(Anonymous class); prototype: Printable<any>.(Anonymous class); message: string; } & typeof Derived>.C; prototype: Tagged<any>.C; } & { new (...any[]): Printable<typeof Derived>.(Anonymous class); prototype: Printable<any>.(Anonymous class); message: string; } & typeof Derived");
AssertType(Tagged, "<T extends Constructor<{}>>(T) => { new (...any[]): C; prototype: Tagged<any>.C; } & T");
AssertType(Printable(Derived), "{ new (...any[]): Printable<typeof Derived>.(Anonymous class); prototype: Printable<any>.(Anonymous class); message: string; } & typeof Derived");
AssertType(Printable, "<T extends Constructor<Base>>(T) => { new (...any[]): (Anonymous class); prototype: Printable<any>.(Anonymous class); message: string; } & T");
AssertType(Derived, "typeof Derived");

Thing2.message;
AssertType(Thing2.message, "string");

function f1() {
    const thing = new Thing1(1, 2, 3);
AssertType(thing, "Tagged<typeof Derived>.C & Derived");
AssertType(new Thing1(1, 2, 3), "Tagged<typeof Derived>.C & Derived");
AssertType(Thing1, "{ new (...any[]): Tagged<typeof Derived>.C; prototype: Tagged<any>.C; } & typeof Derived");
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
AssertType(thing, "Tagged<{ new (...any[]): Printable<typeof Derived>.(Anonymous class); prototype: Printable<any>.(Anonymous class); message: string; } & typeof Derived>.C & Printable<typeof Derived>.(Anonymous class) & Derived");
AssertType(new Thing2(1, 2, 3), "Tagged<{ new (...any[]): Printable<typeof Derived>.(Anonymous class); prototype: Printable<any>.(Anonymous class); message: string; } & typeof Derived>.C & Printable<typeof Derived>.(Anonymous class) & Derived");
AssertType(Thing2, "{ new (...any[]): Tagged<{ new (...any[]): Printable<typeof Derived>.(Anonymous class); prototype: Printable<any>.(Anonymous class); message: string; } & typeof Derived>.C; prototype: Tagged<any>.C; } & { new (...any[]): Printable<typeof Derived>.(Anonymous class); prototype: Printable<any>.(Anonymous class); message: string; } & typeof Derived");
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
AssertType(super, "{ new (...any[]): Tagged<{ new (...any[]): Printable<typeof Derived>.(Anonymous class); prototype: Printable<any>.(Anonymous class); message: string; } & typeof Derived>.C; prototype: Tagged<any>.C; } & { new (...any[]): Printable<typeof Derived>.(Anonymous class); prototype: Printable<any>.(Anonymous class); message: string; } & typeof Derived");
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

// Repro from #13805

const Timestamped = <CT extends Constructor<object>>(Base: CT) => {
AssertType(Timestamped, "<CT extends Constructor<object>>(CT) => { new (...any[]): (Anonymous class); prototype: Timestamped<any>.(Anonymous class); } & CT");
AssertType(Base, "CT");
AssertType(<CT extends Constructor<object>>(Base: CT) => {    return class extends Base {        timestamp = new Date();    };}, "<CT extends Constructor<object>>(CT) => { new (...any[]): (Anonymous class); prototype: Timestamped<any>.(Anonymous class); } & CT");

AssertType(class extends Base {        timestamp = new Date();    }, "{ new (...any[]): (Anonymous class); prototype: Timestamped<any>.(Anonymous class); } & CT");
AssertType(Base, "object");
    return class extends Base {

        timestamp = new Date();
AssertType(timestamp, "Date");
AssertType(new Date(), "Date");
AssertType(Date, "DateConstructor");

    };
}


