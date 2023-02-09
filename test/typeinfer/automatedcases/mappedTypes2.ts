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

// === tests/cases/conformance/types/mapped/mappedTypes2.ts ===
declare function AssertType(value:any, type:string):void;
function verifyLibTypes<T, K extends keyof T, U>() {
    let x1: Partial<T>;
AssertType(x1, "Partial<T>");

    let x1: { [P in keyof T]?: T[P] };
AssertType(x1, "Partial<T>");

    let x2: Readonly<T>;
AssertType(x2, "Readonly<T>");

    let x2: { readonly [P in keyof T]: T[P] };
AssertType(x2, "Readonly<T>");

    let x3: Pick<T, K>;
AssertType(x3, "Pick<T, K>");

    let x3: { [P in K]: T[P] };
AssertType(x3, "Pick<T, K>");

    let x4: Record<K, U>;
AssertType(x4, "Record<K, U>");

    let x4: { [P in K]: U };
AssertType(x4, "Record<K, U>");
}

type Proxy<T> = {
    get(): T;
    set(value: T): void;
}

type Proxify<T> = {
    [P in keyof T]: Proxy<T[P]>;
}

type DeepReadonly<T> = {
    readonly [P in keyof T]: DeepReadonly<T[P]>;
};

declare function assign<T>(obj: T, props: Partial<T>): void;
declare function freeze<T>(obj: T): Readonly<T>;
declare function pick<T, K extends keyof T>(obj: T, ...keys: K[]): Pick<T, K>;
declare function mapObject<K extends string, T, U>(obj: Record<K, T>, f: (x: T) => U): Record<K, U>;
declare function proxify<T>(obj: T): Proxify<T>;

interface Point {
    x: number;
    y: number;
}

interface Shape {
    name: string;
    width: number;
    height: number;
    location: Point;
}

interface PartialShape {
    name?: string;
    width?: number;
    height?: number;
    location?: Point;
}

interface ReadonlyShape {
    readonly name: string;
    readonly width: number;
    readonly height: number;
    readonly location: Point;
}

function f0(s1: Shape, s2: Shape) {
    assign(s1, { name: "circle" });
AssertType(assign(s1, { name: "circle" }), "void");
AssertType(assign, "<T>(T, Partial<T>) => void");
AssertType(s1, "Shape");
AssertType({ name: "circle" }, "{ name: string; }");
AssertType(name, "string");
AssertType("circle", "string");

    assign(s2, { width: 10, height: 20 });
AssertType(assign(s2, { width: 10, height: 20 }), "void");
AssertType(assign, "<T>(T, Partial<T>) => void");
AssertType(s2, "Shape");
AssertType({ width: 10, height: 20 }, "{ width: number; height: number; }");
AssertType(width, "number");
AssertType(10, "int");
AssertType(height, "number");
AssertType(20, "int");
}

function f1(shape: Shape) {
    let frozen: ReadonlyShape;
AssertType(frozen, "ReadonlyShape");

    let frozen: Readonly<Shape>;
AssertType(frozen, "ReadonlyShape");

    let frozen = freeze(shape);
AssertType(frozen, "ReadonlyShape");
AssertType(freeze(shape), "Readonly<Shape>");
AssertType(freeze, "<T>(T) => Readonly<T>");
AssertType(shape, "Shape");
}

function f2(shape: Shape) {
    let partial: PartialShape;
AssertType(partial, "PartialShape");

    let partial: Partial<Shape>;
AssertType(partial, "PartialShape");

    let partial: Partial<Shape> = {};
AssertType(partial, "PartialShape");
AssertType({}, "{}");
}

function f3(shape: Shape) {
    const x = pick(shape, "name", "location");  // { name: string, location: Point 
AssertType(x, "Pick<Shape, union>");

AssertType(pick(shape, "name", "location"), "Pick<Shape, union>");

AssertType(pick, "<T, K extends keyof T>(T, ...K[]) => Pick<T, K>");

AssertType(shape, "Shape");

AssertType("name", "string");

AssertType("location", "string");
}
}

function f4() {
    const rec = { foo: "hello", bar: "world", baz: "bye" };
AssertType(rec, "{ foo: string; bar: string; baz: string; }");
AssertType({ foo: "hello", bar: "world", baz: "bye" }, "{ foo: string; bar: string; baz: string; }");
AssertType(foo, "string");
AssertType("hello", "string");
AssertType(bar, "string");
AssertType("world", "string");
AssertType(baz, "string");
AssertType("bye", "string");

    const lengths = mapObject(rec, s => s.length);  // { foo: number, bar: number, baz: number 
AssertType(lengths, "Record<union, number>");

AssertType(mapObject(rec, s => s.length), "Record<union, number>");

AssertType(mapObject, "<K extends string, T, U>(Record<K, T>, (T) => U) => Record<K, U>");

AssertType(rec, "{ foo: string; bar: string; baz: string; }");

AssertType(s => s.length, "(string) => number");

AssertType(s, "string");

AssertType(s.length, "number");
}
}

function f5(shape: Shape) {
    const p = proxify(shape);
AssertType(p, "Proxify<Shape>");
AssertType(proxify(shape), "Proxify<Shape>");
AssertType(proxify, "<T>(T) => Proxify<T>");
AssertType(shape, "Shape");

    let name = p.name.get();
AssertType(name, "string");
AssertType(p.name.get(), "string");
AssertType(p.name.get, "() => string");
AssertType(p.name, "Proxy<string>");

    p.width.set(42);
AssertType(p.width.set(42), "void");
AssertType(p.width.set, "(number) => void");
AssertType(p.width, "Proxy<number>");
AssertType(42, "int");
}

function f6(shape: DeepReadonly<Shape>) {
    let name = shape.name;  // string
AssertType(name, "string");
AssertType(shape.name, "string");

    let location = shape.location;  // DeepReadonly<Point>
AssertType(location, "DeepReadonly<Point>");
AssertType(shape.location, "DeepReadonly<Point>");

    let x = location.x;  // number
AssertType(x, "number");
AssertType(location.x, "number");
}

