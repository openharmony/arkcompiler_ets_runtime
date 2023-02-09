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

// === tests/cases/conformance/types/typeRelationships/subtypesAndSuperTypes/subtypingWithConstructSignatures4.ts ===
declare function AssertType(value:any, type:string):void;
// checking subtype relations for function types as it relates to contextual signature instantiation

class Base { foo: string; }
class Derived extends Base { bar: string; }
class Derived2 extends Derived { baz: string; }
class OtherDerived extends Base { bing: string; }

declare function foo1(a: new <T>(x: T) => T[]);
declare function foo1(a: any): any;

declare function foo2(a2: new <T>(x: T) => string[]);
declare function foo2(a: any): any;

declare function foo3(a3: new <T>(x: T) => void);
declare function foo3(a: any): any;

declare function foo4(a4: new <T, U>(x: T, y: U) => string);
declare function foo4(a: any): any;

declare function foo5(a5: new <T, U>(x: new (arg: T) => U) => T);
declare function foo5(a: any): any;

declare function foo6(a6: new <T extends Base>(x: new (arg: T) => Derived) => T);
declare function foo6(a: any): any;

declare function foo11(a11: new <T>(x: { foo: T }, y: { foo: T; bar: T }) => Base);
declare function foo11(a: any): any;

declare function foo15(a15: new <T>(x: { a: T; b: T }) => T[]);
declare function foo15(a: any): any;

declare function foo16(a16: new <T extends Base>(x: { a: T; b: T }) => T[]);
declare function foo16(a: any): any;

declare function foo17(a17: {
    new <T extends Derived>(x: new (a: T) => T): T[];
    new <T extends Base>(x: new (a: T) => T): T[];        
});
declare function foo17(a: any): any;

declare function foo18(a18: {
    new (x: {
        new <T extends Derived>(a: T): T;
        new <T extends Base>(a: T): T;
    }): any[];
    new (x: {
        new <T extends Derived2>(a: T): T;
        new <T extends Base>(a: T): T;
    }): any[];
});
declare function foo18(a: any): any;

let r1arg: new <T>(x: T) => T[];
AssertType(r1arg, "new <T>(T) => T[]");
AssertType(x, "T");

let r1arg2: new <T>(x: T) => T[];
AssertType(r1arg2, "new <T>(T) => T[]");
AssertType(x, "T");

let r1 = foo1(r1arg);
AssertType(r1, "any");
AssertType(foo1(r1arg), "any");
AssertType(foo1, "{ (new <T>(T) => T[]): any; (any): any; }");
AssertType(r1arg, "new <T>(T) => T[]");

let r1a = [r1arg, r1arg2];
AssertType(r1a, "(new <T>(T) => T[])[]");
AssertType([r1arg, r1arg2], "(new <T>(T) => T[])[]");
AssertType(r1arg, "new <T>(T) => T[]");
AssertType(r1arg2, "new <T>(T) => T[]");

let r1b = [r1arg2, r1arg];
AssertType(r1b, "(new <T>(T) => T[])[]");
AssertType([r1arg2, r1arg], "(new <T>(T) => T[])[]");
AssertType(r1arg2, "new <T>(T) => T[]");
AssertType(r1arg, "new <T>(T) => T[]");

let r2arg: new <T>(x: T) => string[];
AssertType(r2arg, "new <T>(T) => string[]");
AssertType(x, "T");

let r2arg2: new <T>(x: T) => string[];
AssertType(r2arg2, "new <T>(T) => string[]");
AssertType(x, "T");

let r2 = foo2(r2arg);
AssertType(r2, "any");
AssertType(foo2(r2arg), "any");
AssertType(foo2, "{ (new <T>(T) => string[]): any; (any): any; }");
AssertType(r2arg, "new <T>(T) => string[]");

let r2a = [r2arg, r2arg2];
AssertType(r2a, "(new <T>(T) => string[])[]");
AssertType([r2arg, r2arg2], "(new <T>(T) => string[])[]");
AssertType(r2arg, "new <T>(T) => string[]");
AssertType(r2arg2, "new <T>(T) => string[]");

let r2b = [r2arg2, r2arg];
AssertType(r2b, "(new <T>(T) => string[])[]");
AssertType([r2arg2, r2arg], "(new <T>(T) => string[])[]");
AssertType(r2arg2, "new <T>(T) => string[]");
AssertType(r2arg, "new <T>(T) => string[]");

let r3arg: new <T>(x: T) => T;
AssertType(r3arg, "new <T>(T) => T");
AssertType(x, "T");

let r3arg2: new <T>(x: T) => void;
AssertType(r3arg2, "new <T>(T) => void");
AssertType(x, "T");

let r3 = foo3(r3arg);
AssertType(r3, "any");
AssertType(foo3(r3arg), "any");
AssertType(foo3, "{ (new <T>(T) => void): any; (any): any; }");
AssertType(r3arg, "new <T>(T) => T");

let r3a = [r3arg, r3arg2];
AssertType(r3a, "(new <T>(T) => void)[]");
AssertType([r3arg, r3arg2], "(new <T>(T) => void)[]");
AssertType(r3arg, "new <T>(T) => T");
AssertType(r3arg2, "new <T>(T) => void");

let r3b = [r3arg2, r3arg];
AssertType(r3b, "(new <T>(T) => void)[]");
AssertType([r3arg2, r3arg], "(new <T>(T) => void)[]");
AssertType(r3arg2, "new <T>(T) => void");
AssertType(r3arg, "new <T>(T) => T");

let r4arg: new <T, U>(x: T, y: U) => string;
AssertType(r4arg, "new <T, U>(T, U) => string");
AssertType(x, "T");
AssertType(y, "U");

let r4arg2: new <T, U>(x: T, y: U) => string;
AssertType(r4arg2, "new <T, U>(T, U) => string");
AssertType(x, "T");
AssertType(y, "U");

let r4 = foo4(r4arg);
AssertType(r4, "any");
AssertType(foo4(r4arg), "any");
AssertType(foo4, "{ (new <T, U>(T, U) => string): any; (any): any; }");
AssertType(r4arg, "new <T, U>(T, U) => string");

let r4a = [r4arg, r4arg2];
AssertType(r4a, "(new <T, U>(T, U) => string)[]");
AssertType([r4arg, r4arg2], "(new <T, U>(T, U) => string)[]");
AssertType(r4arg, "new <T, U>(T, U) => string");
AssertType(r4arg2, "new <T, U>(T, U) => string");

let r4b = [r4arg2, r4arg];
AssertType(r4b, "(new <T, U>(T, U) => string)[]");
AssertType([r4arg2, r4arg], "(new <T, U>(T, U) => string)[]");
AssertType(r4arg2, "new <T, U>(T, U) => string");
AssertType(r4arg, "new <T, U>(T, U) => string");

let r5arg: new <T, U>(x: new (arg: T) => U) => T;
AssertType(r5arg, "new <T, U>(new (T) => U) => T");
AssertType(x, "new (T) => U");
AssertType(arg, "T");

let r5arg2: new <T, U>(x: new (arg: T) => U) => T;
AssertType(r5arg2, "new <T, U>(new (T) => U) => T");
AssertType(x, "new (T) => U");
AssertType(arg, "T");

let r5 = foo5(r5arg);
AssertType(r5, "any");
AssertType(foo5(r5arg), "any");
AssertType(foo5, "{ (new <T, U>(new (T) => U) => T): any; (any): any; }");
AssertType(r5arg, "new <T, U>(new (T) => U) => T");

let r5a = [r5arg, r5arg2];
AssertType(r5a, "(new <T, U>(new (T) => U) => T)[]");
AssertType([r5arg, r5arg2], "(new <T, U>(new (T) => U) => T)[]");
AssertType(r5arg, "new <T, U>(new (T) => U) => T");
AssertType(r5arg2, "new <T, U>(new (T) => U) => T");

let r5b = [r5arg2, r5arg];
AssertType(r5b, "(new <T, U>(new (T) => U) => T)[]");
AssertType([r5arg2, r5arg], "(new <T, U>(new (T) => U) => T)[]");
AssertType(r5arg2, "new <T, U>(new (T) => U) => T");
AssertType(r5arg, "new <T, U>(new (T) => U) => T");

let r6arg: new <T extends Base, U extends Derived>(x: new (arg: T) => U) => T;
AssertType(r6arg, "new <T extends Base, U extends Derived>(new (T) => U) => T");
AssertType(x, "new (T) => U");
AssertType(arg, "T");

let r6arg2: new <T extends Base>(x: new (arg: T) => Derived) => T;
AssertType(r6arg2, "new <T extends Base>(new (T) => Derived) => T");
AssertType(x, "new (T) => Derived");
AssertType(arg, "T");

let r6 = foo6(r6arg);
AssertType(r6, "any");
AssertType(foo6(r6arg), "any");
AssertType(foo6, "{ (new <T extends Base>(new (T) => Derived) => T): any; (any): any; }");
AssertType(r6arg, "new <T extends Base, U extends Derived>(new (T) => U) => T");

let r6a = [r6arg, r6arg2];
AssertType(r6a, "(new <T extends Base, U extends Derived>(new (T) => U) => T)[]");
AssertType([r6arg, r6arg2], "(new <T extends Base, U extends Derived>(new (T) => U) => T)[]");
AssertType(r6arg, "new <T extends Base, U extends Derived>(new (T) => U) => T");
AssertType(r6arg2, "new <T extends Base>(new (T) => Derived) => T");

let r6b = [r6arg2, r6arg];
AssertType(r6b, "(new <T extends Base, U extends Derived>(new (T) => U) => T)[]");
AssertType([r6arg2, r6arg], "(new <T extends Base, U extends Derived>(new (T) => U) => T)[]");
AssertType(r6arg2, "new <T extends Base>(new (T) => Derived) => T");
AssertType(r6arg, "new <T extends Base, U extends Derived>(new (T) => U) => T");

let r11arg: new <T, U>(x: { foo: T }, y: { foo: U; bar: U }) => Base;
AssertType(r11arg, "new <T, U>({ foo: T; }, { foo: U; bar: U; }) => Base");
AssertType(x, "{ foo: T; }");
AssertType(foo, "T");
AssertType(y, "{ foo: U; bar: U; }");
AssertType(foo, "U");
AssertType(bar, "U");

let r11arg2: new <T>(x: { foo: T }, y: { foo: T; bar: T }) => Base;
AssertType(r11arg2, "new <T>({ foo: T; }, { foo: T; bar: T; }) => Base");
AssertType(x, "{ foo: T; }");
AssertType(foo, "T");
AssertType(y, "{ foo: T; bar: T; }");
AssertType(foo, "T");
AssertType(bar, "T");

let r11 = foo11(r11arg);
AssertType(r11, "any");
AssertType(foo11(r11arg), "any");
AssertType(foo11, "{ (new <T>({ foo: T; }, { foo: T; bar: T; }) => Base): any; (any): any; }");
AssertType(r11arg, "new <T, U>({ foo: T; }, { foo: U; bar: U; }) => Base");

let r11a = [r11arg, r11arg2];
AssertType(r11a, "(new <T>({ foo: T; }, { foo: T; bar: T; }) => Base)[]");
AssertType([r11arg, r11arg2], "(new <T>({ foo: T; }, { foo: T; bar: T; }) => Base)[]");
AssertType(r11arg, "new <T, U>({ foo: T; }, { foo: U; bar: U; }) => Base");
AssertType(r11arg2, "new <T>({ foo: T; }, { foo: T; bar: T; }) => Base");

let r11b = [r11arg2, r11arg];
AssertType(r11b, "(new <T>({ foo: T; }, { foo: T; bar: T; }) => Base)[]");
AssertType([r11arg2, r11arg], "(new <T>({ foo: T; }, { foo: T; bar: T; }) => Base)[]");
AssertType(r11arg2, "new <T>({ foo: T; }, { foo: T; bar: T; }) => Base");
AssertType(r11arg, "new <T, U>({ foo: T; }, { foo: U; bar: U; }) => Base");

let r15arg: new <U, V>(x: { a: U; b: V; }) => U[];
AssertType(r15arg, "new <U, V>({ a: U; b: V; }) => U[]");
AssertType(x, "{ a: U; b: V; }");
AssertType(a, "U");
AssertType(b, "V");

let r15arg2: new <T>(x: { a: T; b: T }) => T[];
AssertType(r15arg2, "new <T>({ a: T; b: T; }) => T[]");
AssertType(x, "{ a: T; b: T; }");
AssertType(a, "T");
AssertType(b, "T");

let r15 = foo15(r15arg);
AssertType(r15, "any");
AssertType(foo15(r15arg), "any");
AssertType(foo15, "{ (new <T>({ a: T; b: T; }) => T[]): any; (any): any; }");
AssertType(r15arg, "new <U, V>({ a: U; b: V; }) => U[]");

let r15a = [r15arg, r15arg2];
AssertType(r15a, "(new <T>({ a: T; b: T; }) => T[])[]");
AssertType([r15arg, r15arg2], "(new <T>({ a: T; b: T; }) => T[])[]");
AssertType(r15arg, "new <U, V>({ a: U; b: V; }) => U[]");
AssertType(r15arg2, "new <T>({ a: T; b: T; }) => T[]");

let r15b = [r15arg2, r15arg];
AssertType(r15b, "(new <T>({ a: T; b: T; }) => T[])[]");
AssertType([r15arg2, r15arg], "(new <T>({ a: T; b: T; }) => T[])[]");
AssertType(r15arg2, "new <T>({ a: T; b: T; }) => T[]");
AssertType(r15arg, "new <U, V>({ a: U; b: V; }) => U[]");

let r16arg: new <T extends Base>(x: { a: T; b: T }) => T[];
AssertType(r16arg, "new <T extends Base>({ a: T; b: T; }) => T[]");
AssertType(x, "{ a: T; b: T; }");
AssertType(a, "T");
AssertType(b, "T");

let r16arg2: new <T extends Base>(x: { a: T; b: T }) => T[];
AssertType(r16arg2, "new <T extends Base>({ a: T; b: T; }) => T[]");
AssertType(x, "{ a: T; b: T; }");
AssertType(a, "T");
AssertType(b, "T");

let r16 = foo16(r16arg);
AssertType(r16, "any");
AssertType(foo16(r16arg), "any");
AssertType(foo16, "{ (new <T extends Base>({ a: T; b: T; }) => T[]): any; (any): any; }");
AssertType(r16arg, "new <T extends Base>({ a: T; b: T; }) => T[]");

let r16a = [r16arg, r16arg2];
AssertType(r16a, "(new <T extends Base>({ a: T; b: T; }) => T[])[]");
AssertType([r16arg, r16arg2], "(new <T extends Base>({ a: T; b: T; }) => T[])[]");
AssertType(r16arg, "new <T extends Base>({ a: T; b: T; }) => T[]");
AssertType(r16arg2, "new <T extends Base>({ a: T; b: T; }) => T[]");

let r16b = [r16arg2, r16arg];
AssertType(r16b, "(new <T extends Base>({ a: T; b: T; }) => T[])[]");
AssertType([r16arg2, r16arg], "(new <T extends Base>({ a: T; b: T; }) => T[])[]");
AssertType(r16arg2, "new <T extends Base>({ a: T; b: T; }) => T[]");
AssertType(r16arg, "new <T extends Base>({ a: T; b: T; }) => T[]");

let r17arg: new <T>(x: new (a: T) => T) => T[];
AssertType(r17arg, "new <T>(new (T) => T) => T[]");
AssertType(x, "new (T) => T");
AssertType(a, "T");

let r17 = foo17(r17arg);
AssertType(r17, "any");
AssertType(foo17(r17arg), "any");
AssertType(foo17, "{ ({ new <T extends Derived>(new (T) => T): T[]; new <T extends Base>(new (T) => T): T[]; }): any; (any): any; }");
AssertType(r17arg, "new <T>(new (T) => T) => T[]");

let r18arg: new (x: new <T>(a: T) => T) => any[];
AssertType(r18arg, "new (new <T>(T) => T) => any[]");
AssertType(x, "new <T>(T) => T");
AssertType(a, "T");

let r18 = foo18(r18arg);
AssertType(r18, "any");
AssertType(foo18(r18arg), "any");
AssertType(foo18, "{ ({ new ({ new <T extends Derived>(T): T; new <T extends Base>(T): T; }): any[]; new ({ new <T extends Derived2>(T): T; new <T extends Base>(T): T; }): any[]; }): any; (any): any; }");
AssertType(r18arg, "new (new <T>(T) => T) => any[]");


