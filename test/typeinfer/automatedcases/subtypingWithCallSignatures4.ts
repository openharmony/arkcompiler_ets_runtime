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

// === tests/cases/conformance/types/typeRelationships/subtypesAndSuperTypes/subtypingWithCallSignatures4.ts ===
declare function AssertType(value:any, type:string):void;
// checking subtype relations for function types as it relates to contextual signature instantiation

class Base { foo: string; }
class Derived extends Base { bar: string; }
class Derived2 extends Derived { baz: string; }
class OtherDerived extends Base { bing: string; }

declare function foo1(a: <T>(x: T) => T[]);
declare function foo1(a: any): any;

declare function foo2(a2: <T>(x: T) => string[]);
declare function foo2(a: any): any;

declare function foo3(a3: <T>(x: T) => void);
declare function foo3(a: any): any;

declare function foo4(a4: <T, U>(x: T, y: U) => string);
declare function foo4(a: any): any;

declare function foo5(a5: <T, U>(x: (arg: T) => U) => T);
declare function foo5(a: any): any;

declare function foo6(a6: <T extends Base>(x: (arg: T) => Derived) => T);
declare function foo6(a: any): any;

declare function foo11(a11: <T>(x: { foo: T }, y: { foo: T; bar: T }) => Base);
declare function foo11(a: any): any;

declare function foo15(a15: <T>(x: { a: T; b: T }) => T[]);
declare function foo15(a: any): any;

declare function foo16(a16: <T extends Base>(x: { a: T; b: T }) => T[]);
declare function foo16(a: any): any;

declare function foo17(a17: {
    <T extends Derived>(x: (a: T) => T): T[];
    <T extends Base>(x: (a: T) => T): T[];        
});
declare function foo17(a: any): any;

declare function foo18(a18: {
    (x: {
        <T extends Derived>(a: T): T;
        <T extends Base>(a: T): T;
    }): any[];
    (x: {
        <T extends Derived2>(a: T): T;
        <T extends Base>(a: T): T;
    }): any[];
});
declare function foo18(a: any): any;

let r1arg = <T>(x: T) => <T[]>null;
AssertType(r1arg, "<T>(T) => T[]");
AssertType(<T>(x: T) => <T[]>null, "<T>(T) => T[]");
AssertType(x, "T");
AssertType(<T[]>null, "T[]");
AssertType(null, "null");

let r1arg2 = <T>(x: T) => <T[]>null;
AssertType(r1arg2, "<T>(T) => T[]");
AssertType(<T>(x: T) => <T[]>null, "<T>(T) => T[]");
AssertType(x, "T");
AssertType(<T[]>null, "T[]");
AssertType(null, "null");

let r1 = foo1(r1arg);
AssertType(r1, "any");
AssertType(foo1(r1arg), "any");
AssertType(foo1, "{ (<T>(T) => T[]): any; (any): any; }");
AssertType(r1arg, "<T>(T) => T[]");

let r1a = [r1arg, r1arg2];
AssertType(r1a, "(<T>(T) => T[])[]");
AssertType([r1arg, r1arg2], "(<T>(T) => T[])[]");
AssertType(r1arg, "<T>(T) => T[]");
AssertType(r1arg2, "<T>(T) => T[]");

let r1b = [r1arg2, r1arg];
AssertType(r1b, "(<T>(T) => T[])[]");
AssertType([r1arg2, r1arg], "(<T>(T) => T[])[]");
AssertType(r1arg2, "<T>(T) => T[]");
AssertType(r1arg, "<T>(T) => T[]");

let r2arg = <T>(x: T) => [''];
AssertType(r2arg, "<T>(T) => string[]");
AssertType(<T>(x: T) => [''], "<T>(T) => string[]");
AssertType(x, "T");
AssertType([''], "string[]");
AssertType('', "string");

let r2arg2 = <T>(x: T) => [''];
AssertType(r2arg2, "<T>(T) => string[]");
AssertType(<T>(x: T) => [''], "<T>(T) => string[]");
AssertType(x, "T");
AssertType([''], "string[]");
AssertType('', "string");

let r2 = foo2(r2arg);
AssertType(r2, "any");
AssertType(foo2(r2arg), "any");
AssertType(foo2, "{ (<T>(T) => string[]): any; (any): any; }");
AssertType(r2arg, "<T>(T) => string[]");

let r2a = [r2arg, r2arg2];
AssertType(r2a, "(<T>(T) => string[])[]");
AssertType([r2arg, r2arg2], "(<T>(T) => string[])[]");
AssertType(r2arg, "<T>(T) => string[]");
AssertType(r2arg2, "<T>(T) => string[]");

let r2b = [r2arg2, r2arg];
AssertType(r2b, "(<T>(T) => string[])[]");
AssertType([r2arg2, r2arg], "(<T>(T) => string[])[]");
AssertType(r2arg2, "<T>(T) => string[]");
AssertType(r2arg, "<T>(T) => string[]");

let r3arg = <T>(x: T) => <T>null;
AssertType(r3arg, "<T>(T) => T");
AssertType(<T>(x: T) => <T>null, "<T>(T) => T");
AssertType(x, "T");
AssertType(<T>null, "T");
AssertType(null, "null");

let r3arg2 = <T>(x: T) => { };
AssertType(r3arg2, "<T>(T) => void");
AssertType(<T>(x: T) => { }, "<T>(T) => void");
AssertType(x, "T");

let r3 = foo3(r3arg);
AssertType(r3, "any");
AssertType(foo3(r3arg), "any");
AssertType(foo3, "{ (<T>(T) => void): any; (any): any; }");
AssertType(r3arg, "<T>(T) => T");

let r3a = [r3arg, r3arg2];
AssertType(r3a, "(<T>(T) => void)[]");
AssertType([r3arg, r3arg2], "(<T>(T) => void)[]");
AssertType(r3arg, "<T>(T) => T");
AssertType(r3arg2, "<T>(T) => void");

let r3b = [r3arg2, r3arg];
AssertType(r3b, "(<T>(T) => void)[]");
AssertType([r3arg2, r3arg], "(<T>(T) => void)[]");
AssertType(r3arg2, "<T>(T) => void");
AssertType(r3arg, "<T>(T) => T");

let r4arg = <T, U>(x: T, y: U) => '';
AssertType(r4arg, "<T, U>(T, U) => string");
AssertType(<T, U>(x: T, y: U) => '', "<T, U>(T, U) => string");
AssertType(x, "T");
AssertType(y, "U");
AssertType('', "string");

let r4arg2 = <T, U>(x: T, y: U) => '';
AssertType(r4arg2, "<T, U>(T, U) => string");
AssertType(<T, U>(x: T, y: U) => '', "<T, U>(T, U) => string");
AssertType(x, "T");
AssertType(y, "U");
AssertType('', "string");

let r4 = foo4(r4arg);
AssertType(r4, "any");
AssertType(foo4(r4arg), "any");
AssertType(foo4, "{ (<T, U>(T, U) => string): any; (any): any; }");
AssertType(r4arg, "<T, U>(T, U) => string");

let r4a = [r4arg, r4arg2];
AssertType(r4a, "(<T, U>(T, U) => string)[]");
AssertType([r4arg, r4arg2], "(<T, U>(T, U) => string)[]");
AssertType(r4arg, "<T, U>(T, U) => string");
AssertType(r4arg2, "<T, U>(T, U) => string");

let r4b = [r4arg2, r4arg];
AssertType(r4b, "(<T, U>(T, U) => string)[]");
AssertType([r4arg2, r4arg], "(<T, U>(T, U) => string)[]");
AssertType(r4arg2, "<T, U>(T, U) => string");
AssertType(r4arg, "<T, U>(T, U) => string");

let r5arg = <T, U>(x: (arg: T) => U) => <T>null;
AssertType(r5arg, "<T, U>((T) => U) => T");
AssertType(<T, U>(x: (arg: T) => U) => <T>null, "<T, U>((T) => U) => T");
AssertType(x, "(T) => U");
AssertType(arg, "T");
AssertType(<T>null, "T");
AssertType(null, "null");

let r5arg2 = <T, U>(x: (arg: T) => U) => <T>null;
AssertType(r5arg2, "<T, U>((T) => U) => T");
AssertType(<T, U>(x: (arg: T) => U) => <T>null, "<T, U>((T) => U) => T");
AssertType(x, "(T) => U");
AssertType(arg, "T");
AssertType(<T>null, "T");
AssertType(null, "null");

let r5 = foo5(r5arg);
AssertType(r5, "any");
AssertType(foo5(r5arg), "any");
AssertType(foo5, "{ (<T, U>((T) => U) => T): any; (any): any; }");
AssertType(r5arg, "<T, U>((T) => U) => T");

let r5a = [r5arg, r5arg2];
AssertType(r5a, "(<T, U>((T) => U) => T)[]");
AssertType([r5arg, r5arg2], "(<T, U>((T) => U) => T)[]");
AssertType(r5arg, "<T, U>((T) => U) => T");
AssertType(r5arg2, "<T, U>((T) => U) => T");

let r5b = [r5arg2, r5arg];
AssertType(r5b, "(<T, U>((T) => U) => T)[]");
AssertType([r5arg2, r5arg], "(<T, U>((T) => U) => T)[]");
AssertType(r5arg2, "<T, U>((T) => U) => T");
AssertType(r5arg, "<T, U>((T) => U) => T");

let r6arg = <T extends Base, U extends Derived>(x: (arg: T) => U) => <T>null;
AssertType(r6arg, "<T extends Base, U extends Derived>((T) => U) => T");
AssertType(<T extends Base, U extends Derived>(x: (arg: T) => U) => <T>null, "<T extends Base, U extends Derived>((T) => U) => T");
AssertType(x, "(T) => U");
AssertType(arg, "T");
AssertType(<T>null, "T");
AssertType(null, "null");

let r6arg2 = <T extends Base>(x: (arg: T) => Derived) => <T>null;
AssertType(r6arg2, "<T extends Base>((T) => Derived) => T");
AssertType(<T extends Base>(x: (arg: T) => Derived) => <T>null, "<T extends Base>((T) => Derived) => T");
AssertType(x, "(T) => Derived");
AssertType(arg, "T");
AssertType(<T>null, "T");
AssertType(null, "null");

let r6 = foo6(r6arg);
AssertType(r6, "any");
AssertType(foo6(r6arg), "any");
AssertType(foo6, "{ (<T extends Base>((T) => Derived) => T): any; (any): any; }");
AssertType(r6arg, "<T extends Base, U extends Derived>((T) => U) => T");

let r6a = [r6arg, r6arg2];
AssertType(r6a, "(<T extends Base, U extends Derived>((T) => U) => T)[]");
AssertType([r6arg, r6arg2], "(<T extends Base, U extends Derived>((T) => U) => T)[]");
AssertType(r6arg, "<T extends Base, U extends Derived>((T) => U) => T");
AssertType(r6arg2, "<T extends Base>((T) => Derived) => T");

let r6b = [r6arg2, r6arg];
AssertType(r6b, "(<T extends Base, U extends Derived>((T) => U) => T)[]");
AssertType([r6arg2, r6arg], "(<T extends Base, U extends Derived>((T) => U) => T)[]");
AssertType(r6arg2, "<T extends Base>((T) => Derived) => T");
AssertType(r6arg, "<T extends Base, U extends Derived>((T) => U) => T");

let r11arg = <T, U>(x: { foo: T }, y: { foo: U; bar: U }) => <Base>null;
AssertType(r11arg, "<T, U>({ foo: T; }, { foo: U; bar: U; }) => Base");
AssertType(<T, U>(x: { foo: T }, y: { foo: U; bar: U }) => <Base>null, "<T, U>({ foo: T; }, { foo: U; bar: U; }) => Base");
AssertType(x, "{ foo: T; }");
AssertType(foo, "T");
AssertType(y, "{ foo: U; bar: U; }");
AssertType(foo, "U");
AssertType(bar, "U");
AssertType(<Base>null, "Base");
AssertType(null, "null");

let r11arg2 = <T>(x: { foo: T }, y: { foo: T; bar: T }) => <Base>null;
AssertType(r11arg2, "<T>({ foo: T; }, { foo: T; bar: T; }) => Base");
AssertType(<T>(x: { foo: T }, y: { foo: T; bar: T }) => <Base>null, "<T>({ foo: T; }, { foo: T; bar: T; }) => Base");
AssertType(x, "{ foo: T; }");
AssertType(foo, "T");
AssertType(y, "{ foo: T; bar: T; }");
AssertType(foo, "T");
AssertType(bar, "T");
AssertType(<Base>null, "Base");
AssertType(null, "null");

let r11 = foo11(r11arg);
AssertType(r11, "any");
AssertType(foo11(r11arg), "any");
AssertType(foo11, "{ (<T>({ foo: T; }, { foo: T; bar: T; }) => Base): any; (any): any; }");
AssertType(r11arg, "<T, U>({ foo: T; }, { foo: U; bar: U; }) => Base");

let r11a = [r11arg, r11arg2];
AssertType(r11a, "(<T>({ foo: T; }, { foo: T; bar: T; }) => Base)[]");
AssertType([r11arg, r11arg2], "(<T>({ foo: T; }, { foo: T; bar: T; }) => Base)[]");
AssertType(r11arg, "<T, U>({ foo: T; }, { foo: U; bar: U; }) => Base");
AssertType(r11arg2, "<T>({ foo: T; }, { foo: T; bar: T; }) => Base");

let r11b = [r11arg2, r11arg];
AssertType(r11b, "(<T>({ foo: T; }, { foo: T; bar: T; }) => Base)[]");
AssertType([r11arg2, r11arg], "(<T>({ foo: T; }, { foo: T; bar: T; }) => Base)[]");
AssertType(r11arg2, "<T>({ foo: T; }, { foo: T; bar: T; }) => Base");
AssertType(r11arg, "<T, U>({ foo: T; }, { foo: U; bar: U; }) => Base");

let r15arg = <U, V>(x: { a: U; b: V; }) => <U[]>null;
AssertType(r15arg, "<U, V>({ a: U; b: V; }) => U[]");
AssertType(<U, V>(x: { a: U; b: V; }) => <U[]>null, "<U, V>({ a: U; b: V; }) => U[]");
AssertType(x, "{ a: U; b: V; }");
AssertType(a, "U");
AssertType(b, "V");
AssertType(<U[]>null, "U[]");
AssertType(null, "null");

let r15arg2 = <T>(x: { a: T; b: T }) => <T[]>null;
AssertType(r15arg2, "<T>({ a: T; b: T; }) => T[]");
AssertType(<T>(x: { a: T; b: T }) => <T[]>null, "<T>({ a: T; b: T; }) => T[]");
AssertType(x, "{ a: T; b: T; }");
AssertType(a, "T");
AssertType(b, "T");
AssertType(<T[]>null, "T[]");
AssertType(null, "null");

let r15 = foo15(r15arg);
AssertType(r15, "any");
AssertType(foo15(r15arg), "any");
AssertType(foo15, "{ (<T>({ a: T; b: T; }) => T[]): any; (any): any; }");
AssertType(r15arg, "<U, V>({ a: U; b: V; }) => U[]");

let r15a = [r15arg, r15arg2];
AssertType(r15a, "(<T>({ a: T; b: T; }) => T[])[]");
AssertType([r15arg, r15arg2], "(<T>({ a: T; b: T; }) => T[])[]");
AssertType(r15arg, "<U, V>({ a: U; b: V; }) => U[]");
AssertType(r15arg2, "<T>({ a: T; b: T; }) => T[]");

let r15b = [r15arg2, r15arg];
AssertType(r15b, "(<T>({ a: T; b: T; }) => T[])[]");
AssertType([r15arg2, r15arg], "(<T>({ a: T; b: T; }) => T[])[]");
AssertType(r15arg2, "<T>({ a: T; b: T; }) => T[]");
AssertType(r15arg, "<U, V>({ a: U; b: V; }) => U[]");

let r16arg = <T extends Base>(x: { a: T; b: T }) => <T[]>null;
AssertType(r16arg, "<T extends Base>({ a: T; b: T; }) => T[]");
AssertType(<T extends Base>(x: { a: T; b: T }) => <T[]>null, "<T extends Base>({ a: T; b: T; }) => T[]");
AssertType(x, "{ a: T; b: T; }");
AssertType(a, "T");
AssertType(b, "T");
AssertType(<T[]>null, "T[]");
AssertType(null, "null");

let r16arg2 = <T extends Base>(x: { a: T; b: T }) => <T[]>null;
AssertType(r16arg2, "<T extends Base>({ a: T; b: T; }) => T[]");
AssertType(<T extends Base>(x: { a: T; b: T }) => <T[]>null, "<T extends Base>({ a: T; b: T; }) => T[]");
AssertType(x, "{ a: T; b: T; }");
AssertType(a, "T");
AssertType(b, "T");
AssertType(<T[]>null, "T[]");
AssertType(null, "null");

let r16 = foo16(r16arg);
AssertType(r16, "any");
AssertType(foo16(r16arg), "any");
AssertType(foo16, "{ (<T extends Base>({ a: T; b: T; }) => T[]): any; (any): any; }");
AssertType(r16arg, "<T extends Base>({ a: T; b: T; }) => T[]");

let r16a = [r16arg, r16arg2];
AssertType(r16a, "(<T extends Base>({ a: T; b: T; }) => T[])[]");
AssertType([r16arg, r16arg2], "(<T extends Base>({ a: T; b: T; }) => T[])[]");
AssertType(r16arg, "<T extends Base>({ a: T; b: T; }) => T[]");
AssertType(r16arg2, "<T extends Base>({ a: T; b: T; }) => T[]");

let r16b = [r16arg2, r16arg];
AssertType(r16b, "(<T extends Base>({ a: T; b: T; }) => T[])[]");
AssertType([r16arg2, r16arg], "(<T extends Base>({ a: T; b: T; }) => T[])[]");
AssertType(r16arg2, "<T extends Base>({ a: T; b: T; }) => T[]");
AssertType(r16arg, "<T extends Base>({ a: T; b: T; }) => T[]");

let r17arg = <T>(x: (a: T) => T) => <T[]>null;
AssertType(r17arg, "<T>((T) => T) => T[]");
AssertType(<T>(x: (a: T) => T) => <T[]>null, "<T>((T) => T) => T[]");
AssertType(x, "(T) => T");
AssertType(a, "T");
AssertType(<T[]>null, "T[]");
AssertType(null, "null");

let r17 = foo17(r17arg);
AssertType(r17, "any");
AssertType(foo17(r17arg), "any");
AssertType(foo17, "{ ({ <T extends Derived>((T) => T): T[]; <T extends Base>((T) => T): T[]; }): any; (any): any; }");
AssertType(r17arg, "<T>((T) => T) => T[]");

let r18arg = (x: <T>(a: T) => T) => <any[]>null;
AssertType(r18arg, "(<T>(T) => T) => any[]");
AssertType((x: <T>(a: T) => T) => <any[]>null, "(<T>(T) => T) => any[]");
AssertType(x, "<T>(T) => T");
AssertType(a, "T");
AssertType(<any[]>null, "any[]");
AssertType(null, "null");

let r18 = foo18(r18arg);
AssertType(r18, "any");
AssertType(foo18(r18arg), "any");
AssertType(foo18, "{ ({ ({ <T extends Derived>(T): T; <T extends Base>(T): T; }): any[]; ({ <T extends Derived2>(T): T; <T extends Base>(T): T; }): any[]; }): any; (any): any; }");
AssertType(r18arg, "(<T>(T) => T) => any[]");


