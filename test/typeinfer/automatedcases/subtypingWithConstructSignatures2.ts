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

// === tests/cases/conformance/types/typeRelationships/subtypesAndSuperTypes/subtypingWithConstructSignatures2.ts ===
declare function AssertType(value:any, type:string):void;
// checking subtype relations for function types as it relates to contextual signature instantiation

class Base { foo: string; }
class Derived extends Base { bar: string; }
class Derived2 extends Derived { baz: string; }
class OtherDerived extends Base { bing: string; }

declare function foo1(a: new (x: number) => number[]): typeof a;
declare function foo1(a: any): any;

declare function foo2(a: new (x: number) => string[]): typeof a;
declare function foo2(a: any): any;

declare function foo3(a: new (x: number) => void): typeof a;
declare function foo3(a: any): any;

declare function foo4(a: new (x: string, y: number) => string): typeof a;
declare function foo4(a: any): any;

declare function foo5(a: new (x: new (arg: string) => number) => string): typeof a;
declare function foo5(a: any): any;

declare function foo6(a: new (x: new (arg: Base) => Derived) => Base): typeof a;
declare function foo6(a: any): any;

declare function foo7(a: new (x: new (arg: Base) => Derived) => new (r: Base) => Derived): typeof a;
declare function foo7(a: any): any;

declare function foo8(a: new (x: new (arg: Base) => Derived, y: new (arg2: Base) => Derived) => new (r: Base) => Derived): typeof a;
declare function foo8(a: any): any;

declare function foo9(a: new (x: new (arg: Base) => Derived, y: new (arg2: Base) => Derived) => new (r: Base) => Derived): typeof a;
declare function foo9(a: any): any;

declare function foo10(a: new (...x: Derived[]) => Derived): typeof a;
declare function foo10(a: any): any;

declare function foo11(a: new (x: { foo: string }, y: { foo: string; bar: string }) => Base): typeof a;
declare function foo11(a: any): any;

declare function foo12(a: new (x: Array<Base>, y: Array<Derived2>) => Array<Derived>): typeof a;
declare function foo12(a: any): any;

declare function foo13(a: new (x: Array<Base>, y: Array<Derived>) => Array<Derived>): typeof a;
declare function foo13(a: any): any;

declare function foo14(a: new (x: { a: string; b: number }) => Object): typeof a;
declare function foo14(a: any): any;

declare function foo15(a: { 
    new (x: number): number[];
    new (x: string): string[]; 
}): typeof a;
declare function foo15(a: any): any;

declare function foo16(a: {
    new <T extends Derived>(x: T): number[];
    new <U extends Base>(x: U): number[];
}): typeof a;
declare function foo16(a: any): any;

declare function foo17(a: {
    new (x: (a: number) => number): number[];
    new (x: (a: string) => string): string[];
}): typeof a;
declare function foo17(a: any): any;

declare function foo18(a: {
    new (x: {
        new (a: number): number;
        new (a: string): string;
    }): any[];
    new (x: {
        new (a: boolean): boolean;
        new (a: Date): Date;
    }): any[];
}): typeof a;
declare function foo18(a: any): any;

let r1arg1: new <T>(x: T) => T[];
AssertType(r1arg1, "new <T>(T) => T[]");
AssertType(x, "T");

let r1arg2: new (x: number) => number[];
AssertType(r1arg2, "new (number) => number[]");
AssertType(x, "number");

let r1 = foo1(r1arg1); // any, 
AssertType(r1, "new (number) => number[]");
AssertType(foo1(r1arg1), "new (number) => number[]");
AssertType(foo1, "{ (new (number) => number[]): new (number) => number[]; (any): any; }");
AssertType(r1arg1, "new <T>(T) => T[]");
return types are not subtype of first overload

let r1a = [r1arg2, r1arg1]; // generic signature, subtype in both directions
AssertType(r1a, "(new (number) => number[])[]");
AssertType([r1arg2, r1arg1], "(new (number) => number[])[]");
AssertType(r1arg2, "new (number) => number[]");
AssertType(r1arg1, "new <T>(T) => T[]");

let r1b = [r1arg1, r1arg2]; // generic signature, subtype in both directions
AssertType(r1b, "(new (number) => number[])[]");
AssertType([r1arg1, r1arg2], "(new (number) => number[])[]");
AssertType(r1arg1, "new <T>(T) => T[]");
AssertType(r1arg2, "new (number) => number[]");

let r2arg1: new <T>(x: T) => string[];
AssertType(r2arg1, "new <T>(T) => string[]");
AssertType(x, "T");

let r2arg2: new (x: number) => string[];
AssertType(r2arg2, "new (number) => string[]");
AssertType(x, "number");

let r2 = foo2(r2arg1);
AssertType(r2, "new (number) => string[]");
AssertType(foo2(r2arg1), "new (number) => string[]");
AssertType(foo2, "{ (new (number) => string[]): new (number) => string[]; (any): any; }");
AssertType(r2arg1, "new <T>(T) => string[]");

let r2a = [r2arg1, r2arg2];
AssertType(r2a, "(new (number) => string[])[]");
AssertType([r2arg1, r2arg2], "(new (number) => string[])[]");
AssertType(r2arg1, "new <T>(T) => string[]");
AssertType(r2arg2, "new (number) => string[]");

let r2b = [r2arg2, r2arg1];
AssertType(r2b, "(new (number) => string[])[]");
AssertType([r2arg2, r2arg1], "(new (number) => string[])[]");
AssertType(r2arg2, "new (number) => string[]");
AssertType(r2arg1, "new <T>(T) => string[]");

let r3arg1: new <T>(x: T) => T;
AssertType(r3arg1, "new <T>(T) => T");
AssertType(x, "T");

let r3arg2: new (x: number) => void;
AssertType(r3arg2, "new (number) => void");
AssertType(x, "number");

let r3 = foo3(r3arg1);
AssertType(r3, "new (number) => void");
AssertType(foo3(r3arg1), "new (number) => void");
AssertType(foo3, "{ (new (number) => void): new (number) => void; (any): any; }");
AssertType(r3arg1, "new <T>(T) => T");

let r3a = [r3arg1, r3arg2];
AssertType(r3a, "(new (number) => void)[]");
AssertType([r3arg1, r3arg2], "(new (number) => void)[]");
AssertType(r3arg1, "new <T>(T) => T");
AssertType(r3arg2, "new (number) => void");

let r3b = [r3arg2, r3arg1];
AssertType(r3b, "(new (number) => void)[]");
AssertType([r3arg2, r3arg1], "(new (number) => void)[]");
AssertType(r3arg2, "new (number) => void");
AssertType(r3arg1, "new <T>(T) => T");

let r4arg1: new <T, U>(x: T, y: U) => T;
AssertType(r4arg1, "new <T, U>(T, U) => T");
AssertType(x, "T");
AssertType(y, "U");

let r4arg2: new (x: string, y: number) => string;
AssertType(r4arg2, "new (string, number) => string");
AssertType(x, "string");
AssertType(y, "number");

let r4 = foo4(r4arg1); // any
AssertType(r4, "new (string, number) => string");
AssertType(foo4(r4arg1), "new (string, number) => string");
AssertType(foo4, "{ (new (string, number) => string): new (string, number) => string; (any): any; }");
AssertType(r4arg1, "new <T, U>(T, U) => T");

let r4a = [r4arg1, r4arg2];
AssertType(r4a, "(new (string, number) => string)[]");
AssertType([r4arg1, r4arg2], "(new (string, number) => string)[]");
AssertType(r4arg1, "new <T, U>(T, U) => T");
AssertType(r4arg2, "new (string, number) => string");

let r4b = [r4arg2, r4arg1];
AssertType(r4b, "(new (string, number) => string)[]");
AssertType([r4arg2, r4arg1], "(new (string, number) => string)[]");
AssertType(r4arg2, "new (string, number) => string");
AssertType(r4arg1, "new <T, U>(T, U) => T");

let r5arg1: new <T, U>(x: new (arg: T) => U) => T;
AssertType(r5arg1, "new <T, U>(new (T) => U) => T");
AssertType(x, "new (T) => U");
AssertType(arg, "T");

let r5arg2: new (x: new (arg: string) => number) => string;
AssertType(r5arg2, "new (new (string) => number) => string");
AssertType(x, "new (string) => number");
AssertType(arg, "string");

let r5 = foo5(r5arg1); // any
AssertType(r5, "new (new (string) => number) => string");
AssertType(foo5(r5arg1), "new (new (string) => number) => string");
AssertType(foo5, "{ (new (new (string) => number) => string): new (new (string) => number) => string; (any): any; }");
AssertType(r5arg1, "new <T, U>(new (T) => U) => T");

let r5a = [r5arg1, r5arg2];
AssertType(r5a, "(new (new (string) => number) => string)[]");
AssertType([r5arg1, r5arg2], "(new (new (string) => number) => string)[]");
AssertType(r5arg1, "new <T, U>(new (T) => U) => T");
AssertType(r5arg2, "new (new (string) => number) => string");

let r5b = [r5arg2, r5arg1];
AssertType(r5b, "(new (new (string) => number) => string)[]");
AssertType([r5arg2, r5arg1], "(new (new (string) => number) => string)[]");
AssertType(r5arg2, "new (new (string) => number) => string");
AssertType(r5arg1, "new <T, U>(new (T) => U) => T");

let r6arg1: new <T extends Base, U extends Derived>(x: new (arg: T) => U) => T;
AssertType(r6arg1, "new <T extends Base, U extends Derived>(new (T) => U) => T");
AssertType(x, "new (T) => U");
AssertType(arg, "T");

let r6arg2: new (x: new (arg: Base) => Derived) => Base;
AssertType(r6arg2, "new (new (Base) => Derived) => Base");
AssertType(x, "new (Base) => Derived");
AssertType(arg, "Base");

let r6 = foo6(r6arg1); // any
AssertType(r6, "new (new (Base) => Derived) => Base");
AssertType(foo6(r6arg1), "new (new (Base) => Derived) => Base");
AssertType(foo6, "{ (new (new (Base) => Derived) => Base): new (new (Base) => Derived) => Base; (any): any; }");
AssertType(r6arg1, "new <T extends Base, U extends Derived>(new (T) => U) => T");

let r6a = [r6arg1, r6arg2];
AssertType(r6a, "(new (new (Base) => Derived) => Base)[]");
AssertType([r6arg1, r6arg2], "(new (new (Base) => Derived) => Base)[]");
AssertType(r6arg1, "new <T extends Base, U extends Derived>(new (T) => U) => T");
AssertType(r6arg2, "new (new (Base) => Derived) => Base");

let r6b = [r6arg2, r6arg1];
AssertType(r6b, "(new (new (Base) => Derived) => Base)[]");
AssertType([r6arg2, r6arg1], "(new (new (Base) => Derived) => Base)[]");
AssertType(r6arg2, "new (new (Base) => Derived) => Base");
AssertType(r6arg1, "new <T extends Base, U extends Derived>(new (T) => U) => T");

let r7arg1: new <T extends Base, U extends Derived>(x: new (arg: T) => U) => new (r: T) => U;
AssertType(r7arg1, "new <T extends Base, U extends Derived>(new (T) => U) => new (T) => U");
AssertType(x, "new (T) => U");
AssertType(arg, "T");
AssertType(r, "T");

let r7arg2: new (x: new (arg: Base) => Derived) => new (r: Base) => Derived;
AssertType(r7arg2, "new (new (Base) => Derived) => new (Base) => Derived");
AssertType(x, "new (Base) => Derived");
AssertType(arg, "Base");
AssertType(r, "Base");

let r7 = foo7(r7arg1); // any
AssertType(r7, "new (new (Base) => Derived) => new (Base) => Derived");
AssertType(foo7(r7arg1), "new (new (Base) => Derived) => new (Base) => Derived");
AssertType(foo7, "{ (new (new (Base) => Derived) => new (Base) => Derived): new (new (Base) => Derived) => new (Base) => Derived; (any): any; }");
AssertType(r7arg1, "new <T extends Base, U extends Derived>(new (T) => U) => new (T) => U");

let r7a = [r7arg1, r7arg2];
AssertType(r7a, "(new (new (Base) => Derived) => new (Base) => Derived)[]");
AssertType([r7arg1, r7arg2], "(new (new (Base) => Derived) => new (Base) => Derived)[]");
AssertType(r7arg1, "new <T extends Base, U extends Derived>(new (T) => U) => new (T) => U");
AssertType(r7arg2, "new (new (Base) => Derived) => new (Base) => Derived");

let r7b = [r7arg2, r7arg1];
AssertType(r7b, "(new (new (Base) => Derived) => new (Base) => Derived)[]");
AssertType([r7arg2, r7arg1], "(new (new (Base) => Derived) => new (Base) => Derived)[]");
AssertType(r7arg2, "new (new (Base) => Derived) => new (Base) => Derived");
AssertType(r7arg1, "new <T extends Base, U extends Derived>(new (T) => U) => new (T) => U");

let r8arg1: new <T extends Base, U extends Derived>(x: new (arg: T) => U, y: new (arg2: T) => U) => new (r: T) => U;
AssertType(r8arg1, "new <T extends Base, U extends Derived>(new (T) => U, new (T) => U) => new (T) => U");
AssertType(x, "new (T) => U");
AssertType(arg, "T");
AssertType(y, "new (T) => U");
AssertType(arg2, "T");
AssertType(r, "T");

let r8arg2: new (x: new (arg: Base) => Derived, y: new (arg2: Base) => Derived) => new (r: Base) => Derived;
AssertType(r8arg2, "new (new (Base) => Derived, new (Base) => Derived) => new (Base) => Derived");
AssertType(x, "new (Base) => Derived");
AssertType(arg, "Base");
AssertType(y, "new (Base) => Derived");
AssertType(arg2, "Base");
AssertType(r, "Base");

let r8 = foo8(r8arg1); // any
AssertType(r8, "new (new (Base) => Derived, new (Base) => Derived) => new (Base) => Derived");
AssertType(foo8(r8arg1), "new (new (Base) => Derived, new (Base) => Derived) => new (Base) => Derived");
AssertType(foo8, "{ (new (new (Base) => Derived, new (Base) => Derived) => new (Base) => Derived): new (new (Base) => Derived, new (Base) => Derived) => new (Base) => Derived; (any): any; }");
AssertType(r8arg1, "new <T extends Base, U extends Derived>(new (T) => U, new (T) => U) => new (T) => U");

let r8a = [r8arg1, r8arg2];
AssertType(r8a, "(new (new (Base) => Derived, new (Base) => Derived) => new (Base) => Derived)[]");
AssertType([r8arg1, r8arg2], "(new (new (Base) => Derived, new (Base) => Derived) => new (Base) => Derived)[]");
AssertType(r8arg1, "new <T extends Base, U extends Derived>(new (T) => U, new (T) => U) => new (T) => U");
AssertType(r8arg2, "new (new (Base) => Derived, new (Base) => Derived) => new (Base) => Derived");

let r8b = [r8arg2, r8arg1];
AssertType(r8b, "(new (new (Base) => Derived, new (Base) => Derived) => new (Base) => Derived)[]");
AssertType([r8arg2, r8arg1], "(new (new (Base) => Derived, new (Base) => Derived) => new (Base) => Derived)[]");
AssertType(r8arg2, "new (new (Base) => Derived, new (Base) => Derived) => new (Base) => Derived");
AssertType(r8arg1, "new <T extends Base, U extends Derived>(new (T) => U, new (T) => U) => new (T) => U");

let r9arg1: new <T extends Base, U extends Derived>(x: new (arg: T) => U, y: (arg2: { foo: string; bing: number }) => U) => new (r: T) => U;
AssertType(r9arg1, "new <T extends Base, U extends Derived>(new (T) => U, ({    foo: string;    bing: number;}) => U) => new (T) => U");
AssertType(x, "new (T) => U");
AssertType(arg, "T");
AssertType(y, "({    foo: string;    bing: number;}) => U");
AssertType(arg2, "{ foo: string; bing: number; }");
AssertType(foo, "string");
AssertType(bing, "number");
AssertType(r, "T");

let r9arg2: new (x: new (arg: Base) => Derived, y: new (arg2: Base) => Derived) => new (r: Base) => Derived;
AssertType(r9arg2, "new (new (Base) => Derived, new (Base) => Derived) => new (Base) => Derived");
AssertType(x, "new (Base) => Derived");
AssertType(arg, "Base");
AssertType(y, "new (Base) => Derived");
AssertType(arg2, "Base");
AssertType(r, "Base");

let r9 = foo9(r9arg1); // any
AssertType(r9, "any");
AssertType(foo9(r9arg1), "any");
AssertType(foo9, "{ (new (new (Base) => Derived, new (Base) => Derived) => new (Base) => Derived): new (new (Base) => Derived, new (Base) => Derived) => new (Base) => Derived; (any): any; }");
AssertType(r9arg1, "new <T extends Base, U extends Derived>(new (T) => U, ({ foo: string; bing: number; }) => U) => new (T) => U");

let r9a = [r9arg1, r9arg2];
AssertType(r9a, "(union)[]");
AssertType([r9arg1, r9arg2], "(union)[]");
AssertType(r9arg1, "new <T extends Base, U extends Derived>(new (T) => U, ({ foo: string; bing: number; }) => U) => new (T) => U");
AssertType(r9arg2, "new (new (Base) => Derived, new (Base) => Derived) => new (Base) => Derived");

let r9b = [r9arg2, r9arg1];
AssertType(r9b, "(union)[]");
AssertType([r9arg2, r9arg1], "(union)[]");
AssertType(r9arg2, "new (new (Base) => Derived, new (Base) => Derived) => new (Base) => Derived");
AssertType(r9arg1, "new <T extends Base, U extends Derived>(new (T) => U, ({ foo: string; bing: number; }) => U) => new (T) => U");

let r10arg1: new <T extends Derived>(...x: T[]) => T;
AssertType(r10arg1, "new <T extends Derived>(...T[]) => T");
AssertType(x, "T[]");

let r10arg2: new (...x: Derived[]) => Derived;
AssertType(r10arg2, "new (...Derived[]) => Derived");
AssertType(x, "Derived[]");

let r10 = foo10(r10arg1); // any
AssertType(r10, "new (...Derived[]) => Derived");
AssertType(foo10(r10arg1), "new (...Derived[]) => Derived");
AssertType(foo10, "{ (new (...Derived[]) => Derived): new (...Derived[]) => Derived; (any): any; }");
AssertType(r10arg1, "new <T extends Derived>(...T[]) => T");

let r10a = [r10arg1, r10arg2];
AssertType(r10a, "(new (...Derived[]) => Derived)[]");
AssertType([r10arg1, r10arg2], "(new (...Derived[]) => Derived)[]");
AssertType(r10arg1, "new <T extends Derived>(...T[]) => T");
AssertType(r10arg2, "new (...Derived[]) => Derived");

let r10b = [r10arg2, r10arg1];
AssertType(r10b, "(new (...Derived[]) => Derived)[]");
AssertType([r10arg2, r10arg1], "(new (...Derived[]) => Derived)[]");
AssertType(r10arg2, "new (...Derived[]) => Derived");
AssertType(r10arg1, "new <T extends Derived>(...T[]) => T");

let r11arg1: new <T extends Base>(x: T, y: T) => T;
AssertType(r11arg1, "new <T extends Base>(T, T) => T");
AssertType(x, "T");
AssertType(y, "T");

let r11arg2: new (x: { foo: string }, y: { foo: string; bar: string }) => Base;
AssertType(r11arg2, "new ({    foo: string;}, {    foo: string;    bar: string;}) => Base");
AssertType(x, "{ foo: string; }");
AssertType(foo, "string");
AssertType(y, "{ foo: string; bar: string; }");
AssertType(foo, "string");
AssertType(bar, "string");

let r11 = foo11(r11arg1); // any
AssertType(r11, "new ({ foo: string; }, { foo: string; bar: string; }) => Base");
AssertType(foo11(r11arg1), "new ({ foo: string; }, { foo: string; bar: string; }) => Base");
AssertType(foo11, "{ (new ({ foo: string; }, { foo: string; bar: string; }) => Base): new ({ foo: string; }, { foo: string; bar: string; }) => Base; (any): any; }");
AssertType(r11arg1, "new <T extends Base>(T, T) => T");

let r11a = [r11arg1, r11arg2];
AssertType(r11a, "(new ({ foo: string; }, { foo: string; bar: string; }) => Base)[]");
AssertType([r11arg1, r11arg2], "(new ({ foo: string; }, { foo: string; bar: string; }) => Base)[]");
AssertType(r11arg1, "new <T extends Base>(T, T) => T");
AssertType(r11arg2, "new ({ foo: string; }, { foo: string; bar: string; }) => Base");

let r11b = [r11arg2, r11arg1];
AssertType(r11b, "(new ({ foo: string; }, { foo: string; bar: string; }) => Base)[]");
AssertType([r11arg2, r11arg1], "(new ({ foo: string; }, { foo: string; bar: string; }) => Base)[]");
AssertType(r11arg2, "new ({ foo: string; }, { foo: string; bar: string; }) => Base");
AssertType(r11arg1, "new <T extends Base>(T, T) => T");

let r12arg1: new <T extends Array<Base>>(x: Array<Base>, y: T) => Array<Derived>;
AssertType(r12arg1, "new <T extends Base[]>(Array<Base>, T) => Array<Derived>");
AssertType(x, "Base[]");
AssertType(y, "T");

let r12arg2: new (x: Array<Base>, y: Array<Derived2>) => Array<Derived>;
AssertType(r12arg2, "new (Array<Base>, Array<Derived2>) => Array<Derived>");
AssertType(x, "Base[]");
AssertType(y, "Derived2[]");

let r12 = foo12(r12arg1); // any
AssertType(r12, "new (Base[], Derived2[]) => Derived[]");
AssertType(foo12(r12arg1), "new (Base[], Derived2[]) => Derived[]");
AssertType(foo12, "{ (new (Base[], Derived2[]) => Derived[]): new (Base[], Derived2[]) => Derived[]; (any): any; }");
AssertType(r12arg1, "new <T extends Base[]>(Base[], T) => Derived[]");

let r12a = [r12arg1, r12arg2];
AssertType(r12a, "(new (Base[], Derived2[]) => Derived[])[]");
AssertType([r12arg1, r12arg2], "(new (Base[], Derived2[]) => Derived[])[]");
AssertType(r12arg1, "new <T extends Base[]>(Base[], T) => Derived[]");
AssertType(r12arg2, "new (Base[], Derived2[]) => Derived[]");

let r12b = [r12arg2, r12arg1];
AssertType(r12b, "(new (Base[], Derived2[]) => Derived[])[]");
AssertType([r12arg2, r12arg1], "(new (Base[], Derived2[]) => Derived[])[]");
AssertType(r12arg2, "new (Base[], Derived2[]) => Derived[]");
AssertType(r12arg1, "new <T extends Base[]>(Base[], T) => Derived[]");

let r13arg1: new <T extends Array<Derived>>(x: Array<Base>, y: T) => T;
AssertType(r13arg1, "new <T extends Derived[]>(Array<Base>, T) => T");
AssertType(x, "Base[]");
AssertType(y, "T");

let r13arg2: new (x: Array<Base>, y: Array<Derived>) => Array<Derived>;
AssertType(r13arg2, "new (Array<Base>, Array<Derived>) => Array<Derived>");
AssertType(x, "Base[]");
AssertType(y, "Derived[]");

let r13 = foo13(r13arg1); // any
AssertType(r13, "new (Base[], Derived[]) => Derived[]");
AssertType(foo13(r13arg1), "new (Base[], Derived[]) => Derived[]");
AssertType(foo13, "{ (new (Base[], Derived[]) => Derived[]): new (Base[], Derived[]) => Derived[]; (any): any; }");
AssertType(r13arg1, "new <T extends Derived[]>(Base[], T) => T");

let r13a = [r13arg1, r13arg2];
AssertType(r13a, "(new (Base[], Derived[]) => Derived[])[]");
AssertType([r13arg1, r13arg2], "(new (Base[], Derived[]) => Derived[])[]");
AssertType(r13arg1, "new <T extends Derived[]>(Base[], T) => T");
AssertType(r13arg2, "new (Base[], Derived[]) => Derived[]");

let r13b = [r13arg2, r13arg1];
AssertType(r13b, "(new (Base[], Derived[]) => Derived[])[]");
AssertType([r13arg2, r13arg1], "(new (Base[], Derived[]) => Derived[])[]");
AssertType(r13arg2, "new (Base[], Derived[]) => Derived[]");
AssertType(r13arg1, "new <T extends Derived[]>(Base[], T) => T");

let r14arg1: new <T>(x: { a: T; b: T }) => T;
AssertType(r14arg1, "new <T>({ a: T; b: T; }) => T");
AssertType(x, "{ a: T; b: T; }");
AssertType(a, "T");
AssertType(b, "T");

let r14arg2: new (x: { a: string; b: number }) => Object;
AssertType(r14arg2, "new ({    a: string;    b: number;}) => Object");
AssertType(x, "{ a: string; b: number; }");
AssertType(a, "string");
AssertType(b, "number");

let r14 = foo14(r14arg1); // any
AssertType(r14, "any");
AssertType(foo14(r14arg1), "any");
AssertType(foo14, "{ (new ({ a: string; b: number; }) => Object): new ({ a: string; b: number; }) => Object; (any): any; }");
AssertType(r14arg1, "new <T>({ a: T; b: T; }) => T");

let r14a = [r14arg1, r14arg2];
AssertType(r14a, "(union)[]");
AssertType([r14arg1, r14arg2], "(union)[]");
AssertType(r14arg1, "new <T>({ a: T; b: T; }) => T");
AssertType(r14arg2, "new ({ a: string; b: number; }) => Object");

let r14b = [r14arg2, r14arg1];
AssertType(r14b, "(union)[]");
AssertType([r14arg2, r14arg1], "(union)[]");
AssertType(r14arg2, "new ({ a: string; b: number; }) => Object");
AssertType(r14arg1, "new <T>({ a: T; b: T; }) => T");

let r15arg1: new <T>(x: T) => T[];
AssertType(r15arg1, "new <T>(T) => T[]");
AssertType(x, "T");

let r15 = foo15(r15arg1); // any
AssertType(r15, "any");
AssertType(foo15(r15arg1), "any");
AssertType(foo15, "{ ({ new (number): number[]; new (string): string[]; }): { new (number): number[]; new (string): string[]; }; (any): any; }");
AssertType(r15arg1, "new <T>(T) => T[]");

let r16arg1: new <T extends Base>(x: T) => number[];
AssertType(r16arg1, "new <T extends Base>(T) => number[]");
AssertType(x, "T");

let r16 = foo16(r16arg1);
AssertType(r16, "{ new <T extends Derived>(T): number[]; new <U extends Base>(U): number[]; }");
AssertType(foo16(r16arg1), "{ new <T extends Derived>(T): number[]; new <U extends Base>(U): number[]; }");
AssertType(foo16, "{ ({ new <T extends Derived>(T): number[]; new <U extends Base>(U): number[]; }): { new <T extends Derived>(T): number[]; new <U extends Base>(U): number[]; }; (any): any; }");
AssertType(r16arg1, "new <T extends Base>(T) => number[]");

let r17arg1: new <T>(x: (a: T) => T) => T[];
AssertType(r17arg1, "new <T>((T) => T) => T[]");
AssertType(x, "(T) => T");
AssertType(a, "T");

let r17 = foo17(r17arg1); // any
AssertType(r17, "any");
AssertType(foo17(r17arg1), "any");
AssertType(foo17, "{ ({ new ((number) => number): number[]; new ((string) => string): string[]; }): { new ((number) => number): number[]; new ((string) => string): string[]; }; (any): any; }");
AssertType(r17arg1, "new <T>((T) => T) => T[]");

let r18arg1: new <T>(x: (a: T) => T) => T[];
AssertType(r18arg1, "new <T>((T) => T) => T[]");
AssertType(x, "(T) => T");
AssertType(a, "T");

let r18 = foo18(r18arg1); 
AssertType(r18, "any");
AssertType(foo18(r18arg1), "any");
AssertType(foo18, "{ ({ new ({ new (number): number; new (string): string; }): any[]; new ({ new (boolean): boolean; new (Date): Date; }): any[]; }): { new ({ new (number): number; new (string): string; }): any[]; new ({ new (boolean): boolean; new (Date): Date; }): any[]; }; (any): any; }");
AssertType(r18arg1, "new <T>((T) => T) => T[]");


