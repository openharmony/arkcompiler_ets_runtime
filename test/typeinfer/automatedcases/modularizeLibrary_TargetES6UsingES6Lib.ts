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

// === tests/cases/compiler/modularizeLibrary_TargetES6UsingES6Lib.ts ===
declare function AssertType(value:any, type:string):void;
// Using Es6 array
function f(x: number, y: number, z: number) {
AssertType(Array.from(arguments), "any[]");
AssertType(Array.from, "{ <T>(ArrayLike<T>): T[]; <T, U>(ArrayLike<T>, (T, number) => U, ?any): U[]; <T>(union): T[]; <T, U>(union, (T, number) => U, ?any): U[]; }");
AssertType(arguments, "IArguments");
    return Array.from(arguments);
}

f(1, 2, 3);  // no error
AssertType(f(1, 2, 3), "any[]");
AssertType(f, "(number, number, number) => any[]");
AssertType(1, "int");
AssertType(2, "int");
AssertType(3, "int");

// Using ES6 collection
let m = new Map<string, number>();
AssertType(m, "Map<string, number>");
AssertType(new Map<string, number>(), "Map<string, number>");
AssertType(Map, "MapConstructor");

m.clear();
AssertType(m.clear(), "void");
AssertType(m.clear, "() => void");

// Using ES6 iterable
m.keys();
AssertType(m.keys(), "IterableIterator<string>");
AssertType(m.keys, "() => IterableIterator<string>");

// Using ES6 function
function Baz() { }
Baz.name;
AssertType(Baz.name, "string");

// Using ES6 math
Math.sign(1);
AssertType(Math.sign(1), "number");
AssertType(Math.sign, "(number) => number");
AssertType(1, "int");

// Using ES6 object
let o = {
AssertType(o, "{ a: number; [Symbol.hasInstance](any): boolean; }");
AssertType({    a: 2,    [Symbol.hasInstance](value: any) {        return false;    }}, "{ a: number; [Symbol.hasInstance](any): boolean; }");

    a: 2,
AssertType(a, "number");
AssertType(2, "int");

    [Symbol.hasInstance](value: any) {
AssertType([Symbol.hasInstance], "(any) => boolean");
AssertType(Symbol.hasInstance, "unique symbol");
AssertType(value, "any");

AssertType(false, "boolean");
        return false;
    }
};
o.hasOwnProperty(Symbol.hasInstance);
AssertType(o.hasOwnProperty(Symbol.hasInstance), "boolean");
AssertType(o.hasOwnProperty, "(PropertyKey) => boolean");
AssertType(Symbol.hasInstance, "unique symbol");

// Using Es6 proxy
let t = {
AssertType(t, "{}");

AssertType({}, "{}");
}

let p = new Proxy(t, {});
AssertType(p, "{}");
AssertType(new Proxy(t, {}), "{}");
AssertType(Proxy, "ProxyConstructor");
AssertType(t, "{}");
AssertType({}, "{}");

// Using ES6 reflect
Reflect.isExtensible({});
AssertType(Reflect.isExtensible({}), "boolean");
AssertType(Reflect.isExtensible, "(object) => boolean");
AssertType({}, "{}");

// Using Es6 regexp
let reg = new RegExp("/s");
AssertType(reg, "RegExp");
AssertType(new RegExp("/s"), "RegExp");
AssertType(RegExp, "RegExpConstructor");
AssertType("/s", "string");

reg.flags;
AssertType(reg.flags, "string");

// Using ES6 string
let str = "Hello world";
AssertType(str, "string");
AssertType("Hello world", "string");

str.includes("hello", 0);
AssertType(str.includes("hello", 0), "boolean");
AssertType(str.includes, "(string, ?number) => boolean");
AssertType("hello", "string");
AssertType(0, "int");

// Using ES6 symbol
let s = Symbol();
AssertType(s, "symbol");
AssertType(Symbol(), "symbol");
AssertType(Symbol, "SymbolConstructor");

// Using ES6 wellknown-symbol
const o1 = {
AssertType(o1, "{ [Symbol.hasInstance](any): boolean; }");
AssertType({    [Symbol.hasInstance](value: any) {        return false;    }}, "{ [Symbol.hasInstance](any): boolean; }");

    [Symbol.hasInstance](value: any) {
AssertType([Symbol.hasInstance], "(any) => boolean");
AssertType(Symbol.hasInstance, "unique symbol");
AssertType(value, "any");

AssertType(false, "boolean");
        return false;
    }
}

