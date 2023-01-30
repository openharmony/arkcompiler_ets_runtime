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

// === tests/cases/conformance/types/mapped/isomorphicMappedTypeInference.ts ===
declare function AssertType(value:any, type:string):void;
type Box<T> = {
    value: T;
}

type Boxified<T> = {
    [P in keyof T]: Box<T[P]>;
}

function box<T>(x: T): Box<T> {
AssertType({ value: x }, "{ value: T; }");
AssertType(value, "T");
AssertType(x, "T");
    return { value: x };
}

function unbox<T>(x: Box<T>): T {
AssertType(x.value, "T");
    return x.value;
}

function boxify<T>(obj: T): Boxified<T> {
    let result = {} as Boxified<T>;
AssertType(result, "Boxified<T>");
AssertType({} as Boxified<T>, "Boxified<T>");
AssertType({}, "{}");

    for (let k in obj) {
AssertType(k, "Extract<keyof T, string>");
AssertType(obj, "T");

        result[k] = box(obj[k]);
AssertType(result[k] = box(obj[k]), "Box<T[Extract<keyof T, string>]>");
AssertType(result[k], "Boxified<T>[Extract<keyof T, string>]");
AssertType(result, "Boxified<T>");
AssertType(k, "Extract<keyof T, string>");
AssertType(box(obj[k]), "Box<T[Extract<keyof T, string>]>");
AssertType(box, "<T>(T) => Box<T>");
AssertType(obj[k], "T[Extract<keyof T, string>]");
AssertType(obj, "T");
AssertType(k, "Extract<keyof T, string>");
    }
AssertType(result, "Boxified<T>");
    return result;
}

function unboxify<T extends object>(obj: Boxified<T>): T {
    let result = {} as T;
AssertType(result, "T");
AssertType({} as T, "T");
AssertType({}, "{}");

    for (let k in obj) {
AssertType(k, "Extract<keyof T, string>");
AssertType(obj, "Boxified<T>");

        result[k] = unbox(obj[k]);
AssertType(result[k] = unbox(obj[k]), "T[Extract<keyof T, string>]");
AssertType(result[k], "T[Extract<keyof T, string>]");
AssertType(result, "T");
AssertType(k, "Extract<keyof T, string>");
AssertType(unbox(obj[k]), "T[Extract<keyof T, string>]");
AssertType(unbox, "<T>(Box<T>) => T");
AssertType(obj[k], "Boxified<T>[Extract<keyof T, string>]");
AssertType(obj, "Boxified<T>");
AssertType(k, "Extract<keyof T, string>");
    }
AssertType(result, "T");
    return result;
}

function assignBoxified<T>(obj: Boxified<T>, values: T) {
    for (let k in values) {
AssertType(k, "Extract<keyof T, string>");
AssertType(values, "T");

        obj[k].value = values[k];
AssertType(obj[k].value = values[k], "T[Extract<keyof T, string>]");
AssertType(obj[k].value, "T[Extract<keyof T, string>]");
AssertType(obj[k], "Boxified<T>[Extract<keyof T, string>]");
AssertType(obj, "Boxified<T>");
AssertType(k, "Extract<keyof T, string>");
AssertType(values[k], "T[Extract<keyof T, string>]");
AssertType(values, "T");
AssertType(k, "Extract<keyof T, string>");
    }
}

function f1() {
    let v = {
AssertType(v, "{ a: number; b: string; c: boolean; }");
AssertType({        a: 42,        b: "hello",        c: true    }, "{ a: number; b: string; c: boolean; }");

        a: 42,
AssertType(a, "number");
AssertType(42, "int");

        b: "hello",
AssertType(b, "string");
AssertType("hello", "string");

        c: true
AssertType(c, "boolean");
AssertType(true, "boolean");

    };
    let b = boxify(v);
AssertType(b, "Boxified<{ a: number; b: string; c: boolean; }>");
AssertType(boxify(v), "Boxified<{ a: number; b: string; c: boolean; }>");
AssertType(boxify, "<T>(T) => Boxified<T>");
AssertType(v, "{ a: number; b: string; c: boolean; }");

    let x: number = b.a.value;
AssertType(x, "number");
AssertType(b.a.value, "number");
AssertType(b.a, "Box<number>");
}

function f2() {
    let b = {
AssertType(b, "{ a: Box<number>; b: Box<string>; c: Box<boolean>; }");
AssertType({        a: box(42),        b: box("hello"),        c: box(true)    }, "{ a: Box<number>; b: Box<string>; c: Box<boolean>; }");

        a: box(42),
AssertType(a, "Box<number>");
AssertType(box(42), "Box<number>");
AssertType(box, "<T>(T) => Box<T>");
AssertType(42, "int");

        b: box("hello"),
AssertType(b, "Box<string>");
AssertType(box("hello"), "Box<string>");
AssertType(box, "<T>(T) => Box<T>");
AssertType("hello", "string");

        c: box(true)
AssertType(c, "Box<boolean>");
AssertType(box(true), "Box<boolean>");
AssertType(box, "<T>(T) => Box<T>");
AssertType(true, "boolean");

    };
    let v = unboxify(b);
AssertType(v, "{ a: number; b: string; c: boolean; }");
AssertType(unboxify(b), "{ a: number; b: string; c: boolean; }");
AssertType(unboxify, "<T extends object>(Boxified<T>) => T");
AssertType(b, "{ a: Box<number>; b: Box<string>; c: Box<boolean>; }");

    let x: number = v.a;
AssertType(x, "number");
AssertType(v.a, "number");
}

function f3() {
    let b = {
AssertType(b, "{ a: Box<number>; b: Box<string>; c: Box<boolean>; }");
AssertType({        a: box(42),        b: box("hello"),        c: box(true)    }, "{ a: Box<number>; b: Box<string>; c: Box<boolean>; }");

        a: box(42),
AssertType(a, "Box<number>");
AssertType(box(42), "Box<number>");
AssertType(box, "<T>(T) => Box<T>");
AssertType(42, "int");

        b: box("hello"),
AssertType(b, "Box<string>");
AssertType(box("hello"), "Box<string>");
AssertType(box, "<T>(T) => Box<T>");
AssertType("hello", "string");

        c: box(true)
AssertType(c, "Box<boolean>");
AssertType(box(true), "Box<boolean>");
AssertType(box, "<T>(T) => Box<T>");
AssertType(true, "boolean");

    };
    assignBoxified(b, { c: false });
AssertType(assignBoxified(b, { c: false }), "void");
AssertType(assignBoxified, "<T>(Boxified<T>, T) => void");
AssertType(b, "{ a: Box<number>; b: Box<string>; c: Box<boolean>; }");
AssertType({ c: false }, "{ c: false; }");
AssertType(c, "boolean");
AssertType(false, "boolean");
}

function f4() {
    let b = {
AssertType(b, "{ a: Box<number>; b: Box<string>; c: Box<boolean>; }");
AssertType({        a: box(42),        b: box("hello"),        c: box(true)    }, "{ a: Box<number>; b: Box<string>; c: Box<boolean>; }");

        a: box(42),
AssertType(a, "Box<number>");
AssertType(box(42), "Box<number>");
AssertType(box, "<T>(T) => Box<T>");
AssertType(42, "int");

        b: box("hello"),
AssertType(b, "Box<string>");
AssertType(box("hello"), "Box<string>");
AssertType(box, "<T>(T) => Box<T>");
AssertType("hello", "string");

        c: box(true)
AssertType(c, "Box<boolean>");
AssertType(box(true), "Box<boolean>");
AssertType(box, "<T>(T) => Box<T>");
AssertType(true, "boolean");

    };
    b = boxify(unboxify(b));
AssertType(b = boxify(unboxify(b)), "Boxified<{ a: number; b: string; c: boolean; }>");
AssertType(b, "{ a: Box<number>; b: Box<string>; c: Box<boolean>; }");
AssertType(boxify(unboxify(b)), "Boxified<{ a: number; b: string; c: boolean; }>");
AssertType(boxify, "<T>(T) => Boxified<T>");
AssertType(unboxify(b), "{ a: number; b: string; c: boolean; }");
AssertType(unboxify, "<T extends object>(Boxified<T>) => T");
AssertType(b, "{ a: Box<number>; b: Box<string>; c: Box<boolean>; }");

    b = unboxify(boxify(b));
AssertType(b = unboxify(boxify(b)), "{ a: Box<number>; b: Box<string>; c: Box<boolean>; }");
AssertType(b, "{ a: Box<number>; b: Box<string>; c: Box<boolean>; }");
AssertType(unboxify(boxify(b)), "{ a: Box<number>; b: Box<string>; c: Box<boolean>; }");
AssertType(unboxify, "<T extends object>(Boxified<T>) => T");
AssertType(boxify(b), "Boxified<{ a: Box<number>; b: Box<string>; c: Box<boolean>; }>");
AssertType(boxify, "<T>(T) => Boxified<T>");
AssertType(b, "{ a: Box<number>; b: Box<string>; c: Box<boolean>; }");
}

function makeRecord<T, K extends string>(obj: { [P in K]: T }) {
AssertType(obj, "{ [P in K]: T; }");
    return obj;
}

function f5(s: string) {
    let b = makeRecord({
AssertType(b, "{ a: union; b: union; c: union; }");
AssertType(makeRecord({        a: box(42),        b: box("hello"),        c: box(true)    }), "{ a: union; b: union; c: union; }");
AssertType(makeRecord, "<T, K extends string>({ [P in K]: T; }) => { [P in K]: T; }");
AssertType({        a: box(42),        b: box("hello"),        c: box(true)    }, "{ a: Box<number>; b: Box<string>; c: Box<boolean>; }");

        a: box(42),
AssertType(a, "Box<number>");
AssertType(box(42), "Box<number>");
AssertType(box, "<T>(T) => Box<T>");
AssertType(42, "int");

        b: box("hello"),
AssertType(b, "Box<string>");
AssertType(box("hello"), "Box<string>");
AssertType(box, "<T>(T) => Box<T>");
AssertType("hello", "string");

        c: box(true)
AssertType(c, "Box<boolean>");
AssertType(box(true), "Box<boolean>");
AssertType(box, "<T>(T) => Box<T>");
AssertType(true, "boolean");

    });
    let v = unboxify(b);
AssertType(v, "{ a: union; b: union; c: union; }");
AssertType(unboxify(b), "{ a: union; b: union; c: union; }");
AssertType(unboxify, "<T extends object>(Boxified<T>) => T");
AssertType(b, "{ a: union; b: union; c: union; }");

    let x: string | number | boolean = v.a;
AssertType(x, "union");
AssertType(v.a, "union");
}

function makeDictionary<T>(obj: { [x: string]: T }) {
AssertType(obj, "{ [string]: T; }");
    return obj;
}

function f6(s: string) {
    let b = makeDictionary({
AssertType(b, "{ [string]: union; }");
AssertType(makeDictionary({        a: box(42),        b: box("hello"),        c: box(true)    }), "{ [string]: union; }");
AssertType(makeDictionary, "<T>({ [string]: T; }) => { [string]: T; }");
AssertType({        a: box(42),        b: box("hello"),        c: box(true)    }, "{ a: Box<number>; b: Box<string>; c: Box<boolean>; }");

        a: box(42),
AssertType(a, "Box<number>");
AssertType(box(42), "Box<number>");
AssertType(box, "<T>(T) => Box<T>");
AssertType(42, "int");

        b: box("hello"),
AssertType(b, "Box<string>");
AssertType(box("hello"), "Box<string>");
AssertType(box, "<T>(T) => Box<T>");
AssertType("hello", "string");

        c: box(true)
AssertType(c, "Box<boolean>");
AssertType(box(true), "Box<boolean>");
AssertType(box, "<T>(T) => Box<T>");
AssertType(true, "boolean");

    });
    let v = unboxify(b);
AssertType(v, "{ [string]: any; }");
AssertType(unboxify(b), "{ [string]: any; }");
AssertType(unboxify, "<T extends object>(Boxified<T>) => T");
AssertType(b, "{ [string]: union; }");

    let x: string | number | boolean = v[s];
AssertType(x, "union");
AssertType(v[s], "union");
AssertType(v, "{ [string]: any; }");
AssertType(s, "string");
}

declare function validate<T>(obj: { [P in keyof T]?: T[P] }): T;
declare function clone<T>(obj: { readonly [P in keyof T]: T[P] }): T;
declare function validateAndClone<T>(obj: { readonly [P in keyof T]?: T[P] }): T;

type Foo = {
    a?: number;
    readonly b: string;
}

function f10(foo: Foo) {
    let x = validate(foo);  // { a: number, readonly b: string 
AssertType(x, "{ a: number; readonly b: string; }");

AssertType(validate(foo), "{ a: number; readonly b: string; }");

AssertType(validate, "<T>({ [P in keyof T]?: union; }) => T");

AssertType(foo, "Foo");
}

    let y = clone(foo);  // { a?: number, b: string 
AssertType(y, "{ a?: union; b: string; }");

AssertType(clone(foo), "{ a?: union; b: string; }");

AssertType(clone, "<T>({ readonly [P in keyof T]: T[P]; }) => T");

AssertType(foo, "Foo");
}

    let z = validateAndClone(foo);  // { a: number, b: string 
AssertType(z, "{ a: number; b: string; }");

AssertType(validateAndClone(foo), "{ a: number; b: string; }");

AssertType(validateAndClone, "<T>({ readonly [P in keyof T]?: union; }) => T");

AssertType(foo, "Foo");
}
}

// Repro from #12606

type Func<T> = (...args: any[]) => T;
type Spec<T> = {
    [P in keyof T]: Func<T[P]> | Spec<T[P]> ;
};

/**
 * Given a spec object recursively mapping properties to functions, creates a function
 * producing an object of the same structure, by mapping each property to the result
 * of calling its associated function with the supplied arguments.
 */
declare function applySpec<T>(obj: Spec<T>): (...args: any[]) => T;

// Infers g1: (...args: any[]) => { sum: number, nested: { mul: string } }
let g1 = applySpec({
AssertType(g1, "(...any[]) => { sum: number; nested: { mul: string; }; }");
AssertType(applySpec({    sum: (a: any) => 3,    nested: {        mul: (b: any) => "n"    }}), "(...any[]) => { sum: number; nested: { mul: string; }; }");
AssertType(applySpec, "<T>(Spec<T>) => (...any[]) => T");
AssertType({    sum: (a: any) => 3,    nested: {        mul: (b: any) => "n"    }}, "{ sum: (any) => number; nested: { mul: (any) => string; }; }");

    sum: (a: any) => 3,
AssertType(sum, "(any) => number");
AssertType((a: any) => 3, "(any) => number");
AssertType(a, "any");
AssertType(3, "int");

    nested: {
AssertType(nested, "{ mul: (any) => string; }");
AssertType({        mul: (b: any) => "n"    }, "{ mul: (any) => string; }");

        mul: (b: any) => "n"
AssertType(mul, "(any) => string");
AssertType((b: any) => "n", "(any) => string");
AssertType(b, "any");
AssertType("n", "string");
    }
});

// Infers g2: (...args: any[]) => { foo: { bar: { baz: boolean } } }
let g2 = applySpec({ foo: { bar: { baz: (x: any) => true } } });
AssertType(g2, "(...any[]) => { foo: { bar: { baz: boolean; }; }; }");
AssertType(applySpec({ foo: { bar: { baz: (x: any) => true } } }), "(...any[]) => { foo: { bar: { baz: boolean; }; }; }");
AssertType(applySpec, "<T>(Spec<T>) => (...any[]) => T");
AssertType({ foo: { bar: { baz: (x: any) => true } } }, "{ foo: { bar: { baz: (any) => boolean; }; }; }");
AssertType(foo, "{ bar: { baz: (any) => boolean; }; }");
AssertType({ bar: { baz: (x: any) => true } }, "{ bar: { baz: (any) => boolean; }; }");
AssertType(bar, "{ baz: (any) => boolean; }");
AssertType({ baz: (x: any) => true }, "{ baz: (any) => boolean; }");
AssertType(baz, "(any) => boolean");
AssertType((x: any) => true, "(any) => boolean");
AssertType(x, "any");
AssertType(true, "boolean");

// Repro from #12633

const foo = <T>(object: T, partial: Partial<T>) => object;
AssertType(foo, "<T>(T, Partial<T>) => T");
AssertType(<T>(object: T, partial: Partial<T>) => object, "<T>(T, Partial<T>) => T");
AssertType(object, "T");
AssertType(partial, "Partial<T>");
AssertType(object, "T");

let o = {a: 5, b: 7};
AssertType(o, "{ a: number; b: number; }");
AssertType({a: 5, b: 7}, "{ a: number; b: number; }");
AssertType(a, "number");
AssertType(5, "int");
AssertType(b, "number");
AssertType(7, "int");

foo(o, {b: 9});
AssertType(foo(o, {b: 9}), "{ a: number; b: number; }");
AssertType(foo, "<T>(T, Partial<T>) => T");
AssertType(o, "{ a: number; b: number; }");
AssertType({b: 9}, "{ b: number; }");
AssertType(b, "number");
AssertType(9, "int");

o = foo(o, {b: 9});
AssertType(o = foo(o, {b: 9}), "{ a: number; b: number; }");
AssertType(o, "{ a: number; b: number; }");
AssertType(foo(o, {b: 9}), "{ a: number; b: number; }");
AssertType(foo, "<T>(T, Partial<T>) => T");
AssertType(o, "{ a: number; b: number; }");
AssertType({b: 9}, "{ b: number; }");
AssertType(b, "number");
AssertType(9, "int");

// Inferring to { [P in K]: X }, where K extends keyof T, produces same inferences as
// inferring to { [P in keyof T]: X }.

declare function f20<T, K extends keyof T>(obj: Pick<T, K>): T;
declare function f21<T, K extends keyof T>(obj: Pick<T, K>): K;
declare function f22<T, K extends keyof T>(obj: Boxified<Pick<T, K>>): T;
declare function f23<T, U extends keyof T, K extends U>(obj: Pick<T, K>): T;
declare function f24<T, U, K extends keyof T | keyof U>(obj: Pick<T & U, K>): T & U;

let x0 = f20({ foo: 42, bar: "hello" });
AssertType(x0, "{ foo: number; bar: string; }");
AssertType(f20({ foo: 42, bar: "hello" }), "{ foo: number; bar: string; }");
AssertType(f20, "<T, K extends keyof T>(Pick<T, K>) => T");
AssertType({ foo: 42, bar: "hello" }, "{ foo: number; bar: string; }");
AssertType(foo, "number");
AssertType(42, "int");
AssertType(bar, "string");
AssertType("hello", "string");

let x1 = f21({ foo: 42, bar: "hello" });
AssertType(x1, "union");
AssertType(f21({ foo: 42, bar: "hello" }), "union");
AssertType(f21, "<T, K extends keyof T>(Pick<T, K>) => K");
AssertType({ foo: 42, bar: "hello" }, "{ foo: number; bar: string; }");
AssertType(foo, "number");
AssertType(42, "int");
AssertType(bar, "string");
AssertType("hello", "string");

let x2 = f22({ foo: { value: 42} , bar: { value: "hello" } });
AssertType(x2, "{ foo: number; bar: string; }");
AssertType(f22({ foo: { value: 42} , bar: { value: "hello" } }), "{ foo: number; bar: string; }");
AssertType(f22, "<T, K extends keyof T>(Boxified<Pick<T, K>>) => T");
AssertType({ foo: { value: 42} , bar: { value: "hello" } }, "{ foo: { value: number; }; bar: { value: string; }; }");
AssertType(foo, "{ value: number; }");
AssertType({ value: 42}, "{ value: number; }");
AssertType(value, "number");
AssertType(42, "int");
AssertType(bar, "{ value: string; }");
AssertType({ value: "hello" }, "{ value: string; }");
AssertType(value, "string");
AssertType("hello", "string");

let x3 = f23({ foo: 42, bar: "hello" });
AssertType(x3, "{ foo: number; bar: string; }");
AssertType(f23({ foo: 42, bar: "hello" }), "{ foo: number; bar: string; }");
AssertType(f23, "<T, U extends keyof T, K extends U>(Pick<T, K>) => T");
AssertType({ foo: 42, bar: "hello" }, "{ foo: number; bar: string; }");
AssertType(foo, "number");
AssertType(42, "int");
AssertType(bar, "string");
AssertType("hello", "string");

let x4 = f24({ foo: 42, bar: "hello" });
AssertType(x4, "{ foo: number; bar: string; } & { foo: number; bar: string; }");
AssertType(f24({ foo: 42, bar: "hello" }), "{ foo: number; bar: string; } & { foo: number; bar: string; }");
AssertType(f24, "<T, U, K extends union>(Pick<T & U, K>) => T & U");
AssertType({ foo: 42, bar: "hello" }, "{ foo: number; bar: string; }");
AssertType(foo, "number");
AssertType(42, "int");
AssertType(bar, "string");
AssertType("hello", "string");

// Repro from #29765

function getProps<T, K extends keyof T>(obj: T, list: K[]): Pick<T, K> {
AssertType({} as any, "any");
AssertType({}, "{}");
    return {} as any;
}

const myAny: any = {};
AssertType(myAny, "any");
AssertType({}, "{}");

const o1 = getProps(myAny, ['foo', 'bar']);
AssertType(o1, "Pick<any, union>");
AssertType(getProps(myAny, ['foo', 'bar']), "Pick<any, union>");
AssertType(getProps, "<T, K extends keyof T>(T, K[]) => Pick<T, K>");
AssertType(myAny, "any");
AssertType(['foo', 'bar'], "(union)[]");
AssertType('foo', "string");
AssertType('bar', "string");

const o2: { foo: any; bar: any } = getProps(myAny, ['foo', 'bar']);
AssertType(o2, "{ foo: any; bar: any; }");
AssertType(foo, "any");
AssertType(bar, "any");
AssertType(getProps(myAny, ['foo', 'bar']), "Pick<any, union>");
AssertType(getProps, "<T, K extends keyof T>(T, K[]) => Pick<T, K>");
AssertType(myAny, "any");
AssertType(['foo', 'bar'], "(union)[]");
AssertType('foo', "string");
AssertType('bar', "string");


