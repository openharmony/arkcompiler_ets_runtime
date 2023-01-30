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

// === tests/cases/conformance/types/typeRelationships/subtypesAndSuperTypes/subtypingWithCallSignatures2.ts ===
declare function AssertType(value:any, type:string):void;
// checking subtype relations for function types as it relates to contextual signature instantiation

class Base { foo: string; }
class Derived extends Base { bar: string; }
class Derived2 extends Derived { baz: string; }
class OtherDerived extends Base { bing: string; }

declare function foo1(a: (x: number) => number[]): typeof a;
declare function foo1(a: any): any;

declare function foo2(a: (x: number) => string[]): typeof a;
declare function foo2(a: any): any;

declare function foo3(a: (x: number) => void): typeof a;
declare function foo3(a: any): any;

declare function foo4(a: (x: string, y: number) => string): typeof a;
declare function foo4(a: any): any;

declare function foo5(a: (x: (arg: string) => number) => string): typeof a;
declare function foo5(a: any): any;

declare function foo6(a: (x: (arg: Base) => Derived) => Base): typeof a;
declare function foo6(a: any): any;

declare function foo7(a: (x: (arg: Base) => Derived) => (r: Base) => Derived): typeof a;
declare function foo7(a: any): any;

declare function foo8(a: (x: (arg: Base) => Derived, y: (arg2: Base) => Derived) => (r: Base) => Derived): typeof a;
declare function foo8(a: any): any;

declare function foo9(a: (x: (arg: Base) => Derived, y: (arg2: Base) => Derived) => (r: Base) => Derived): typeof a;
declare function foo9(a: any): any;

declare function foo10(a: (...x: Derived[]) => Derived): typeof a;
declare function foo10(a: any): any;

declare function foo11(a: (x: { foo: string }, y: { foo: string; bar: string }) => Base): typeof a;
declare function foo11(a: any): any;

declare function foo12(a: (x: Array<Base>, y: Array<Derived2>) => Array<Derived>): typeof a;
declare function foo12(a: any): any;

declare function foo13(a: (x: Array<Base>, y: Array<Derived>) => Array<Derived>): typeof a;
declare function foo13(a: any): any;

declare function foo14(a: (x: { a: string; b: number }) => Object): typeof a;
declare function foo14(a: any): any;

declare function foo15(a: { 
    (x: number): number[];
    (x: string): string[]; 
}): typeof a;
declare function foo15(a: any): any;

declare function foo16(a: {
    <T extends Derived>(x: T): number[];
    <U extends Base>(x: U): number[];
}): typeof a;
declare function foo16(a: any): any;

declare function foo17(a: {
    (x: (a: number) => number): number[];
    (x: (a: string) => string): string[];
}): typeof a;
declare function foo17(a: any): any;

declare function foo18(a: {
    (x: {
        (a: number): number;
        (a: string): string;
    }): any[];
    (x: {
        (a: boolean): boolean;
        (a: Date): Date;
    }): any[];
}): typeof a;
declare function foo18(a: any): any;

let r1arg1 = <T>(x: T) => [x];
AssertType(r1arg1, "<T>(T) => T[]");
AssertType(<T>(x: T) => [x], "<T>(T) => T[]");
AssertType(x, "T");
AssertType([x], "T[]");
AssertType(x, "T");

let r1arg2 = (x: number) => [1];
AssertType(r1arg2, "(number) => number[]");
AssertType((x: number) => [1], "(number) => number[]");
AssertType(x, "number");
AssertType([1], "number[]");
AssertType(1, "int");

let r1 = foo1(r1arg1); // any, 
AssertType(r1, "(number) => number[]");
AssertType(foo1(r1arg1), "(number) => number[]");
AssertType(foo1, "{ ((number) => number[]): (number) => number[]; (any): any; }");
AssertType(r1arg1, "<T>(T) => T[]");
return types are not subtype of first overload

let r1a = [r1arg2, r1arg1]; // generic signature, subtype in both directions
AssertType(r1a, "((number) => number[])[]");
AssertType([r1arg2, r1arg1], "((number) => number[])[]");
AssertType(r1arg2, "(number) => number[]");
AssertType(r1arg1, "<T>(T) => T[]");

let r1b = [r1arg1, r1arg2]; // generic signature, subtype in both directions
AssertType(r1b, "((number) => number[])[]");
AssertType([r1arg1, r1arg2], "((number) => number[])[]");
AssertType(r1arg1, "<T>(T) => T[]");
AssertType(r1arg2, "(number) => number[]");

let r2arg1 = <T>(x: T) => [''];
AssertType(r2arg1, "<T>(T) => string[]");
AssertType(<T>(x: T) => [''], "<T>(T) => string[]");
AssertType(x, "T");
AssertType([''], "string[]");
AssertType('', "string");

let r2arg2 = (x: number) => [''];
AssertType(r2arg2, "(number) => string[]");
AssertType((x: number) => [''], "(number) => string[]");
AssertType(x, "number");
AssertType([''], "string[]");
AssertType('', "string");

let r2 = foo2(r2arg1); 
AssertType(r2, "(number) => string[]");
AssertType(foo2(r2arg1), "(number) => string[]");
AssertType(foo2, "{ ((number) => string[]): (number) => string[]; (any): any; }");
AssertType(r2arg1, "<T>(T) => string[]");

let r2a = [r2arg1, r2arg2];
AssertType(r2a, "((number) => string[])[]");
AssertType([r2arg1, r2arg2], "((number) => string[])[]");
AssertType(r2arg1, "<T>(T) => string[]");
AssertType(r2arg2, "(number) => string[]");

let r2b = [r2arg2, r2arg1];
AssertType(r2b, "((number) => string[])[]");
AssertType([r2arg2, r2arg1], "((number) => string[])[]");
AssertType(r2arg2, "(number) => string[]");
AssertType(r2arg1, "<T>(T) => string[]");

let r3arg1 = <T>(x: T) => x;
AssertType(r3arg1, "<T>(T) => T");
AssertType(<T>(x: T) => x, "<T>(T) => T");
AssertType(x, "T");
AssertType(x, "T");

let r3arg2 = (x: number) => { };
AssertType(r3arg2, "(number) => void");
AssertType((x: number) => { }, "(number) => void");
AssertType(x, "number");

let r3 = foo3(r3arg1); 
AssertType(r3, "(number) => void");
AssertType(foo3(r3arg1), "(number) => void");
AssertType(foo3, "{ ((number) => void): (number) => void; (any): any; }");
AssertType(r3arg1, "<T>(T) => T");

let r3a = [r3arg1, r3arg2];
AssertType(r3a, "((number) => void)[]");
AssertType([r3arg1, r3arg2], "((number) => void)[]");
AssertType(r3arg1, "<T>(T) => T");
AssertType(r3arg2, "(number) => void");

let r3b = [r3arg2, r3arg1];
AssertType(r3b, "((number) => void)[]");
AssertType([r3arg2, r3arg1], "((number) => void)[]");
AssertType(r3arg2, "(number) => void");
AssertType(r3arg1, "<T>(T) => T");

let r4arg1 = <T, U>(x: T, y: U) => x;
AssertType(r4arg1, "<T, U>(T, U) => T");
AssertType(<T, U>(x: T, y: U) => x, "<T, U>(T, U) => T");
AssertType(x, "T");
AssertType(y, "U");
AssertType(x, "T");

let r4arg2 = (x: string, y: number) => '';
AssertType(r4arg2, "(string, number) => string");
AssertType((x: string, y: number) => '', "(string, number) => string");
AssertType(x, "string");
AssertType(y, "number");
AssertType('', "string");

let r4 = foo4(r4arg1); // any
AssertType(r4, "(string, number) => string");
AssertType(foo4(r4arg1), "(string, number) => string");
AssertType(foo4, "{ ((string, number) => string): (string, number) => string; (any): any; }");
AssertType(r4arg1, "<T, U>(T, U) => T");

let r4a = [r4arg1, r4arg2];
AssertType(r4a, "((string, number) => string)[]");
AssertType([r4arg1, r4arg2], "((string, number) => string)[]");
AssertType(r4arg1, "<T, U>(T, U) => T");
AssertType(r4arg2, "(string, number) => string");

let r4b = [r4arg2, r4arg1];
AssertType(r4b, "((string, number) => string)[]");
AssertType([r4arg2, r4arg1], "((string, number) => string)[]");
AssertType(r4arg2, "(string, number) => string");
AssertType(r4arg1, "<T, U>(T, U) => T");

let r5arg1 = <T, U>(x: (arg: T) => U) => <T>null;
AssertType(r5arg1, "<T, U>((T) => U) => T");
AssertType(<T, U>(x: (arg: T) => U) => <T>null, "<T, U>((T) => U) => T");
AssertType(x, "(T) => U");
AssertType(arg, "T");
AssertType(<T>null, "T");
AssertType(null, "null");

let r5arg2 = (x: (arg: string) => number) => '';
AssertType(r5arg2, "((string) => number) => string");
AssertType((x: (arg: string) => number) => '', "((string) => number) => string");
AssertType(x, "(string) => number");
AssertType(arg, "string");
AssertType('', "string");

let r5 = foo5(r5arg1); // any
AssertType(r5, "((string) => number) => string");
AssertType(foo5(r5arg1), "((string) => number) => string");
AssertType(foo5, "{ (((string) => number) => string): ((string) => number) => string; (any): any; }");
AssertType(r5arg1, "<T, U>((T) => U) => T");

let r5a = [r5arg1, r5arg2];
AssertType(r5a, "(((string) => number) => string)[]");
AssertType([r5arg1, r5arg2], "(((string) => number) => string)[]");
AssertType(r5arg1, "<T, U>((T) => U) => T");
AssertType(r5arg2, "((string) => number) => string");

let r5b = [r5arg2, r5arg1];
AssertType(r5b, "(((string) => number) => string)[]");
AssertType([r5arg2, r5arg1], "(((string) => number) => string)[]");
AssertType(r5arg2, "((string) => number) => string");
AssertType(r5arg1, "<T, U>((T) => U) => T");

let r6arg1 = <T extends Base, U extends Derived>(x: (arg: T) => U) => <T>null;
AssertType(r6arg1, "<T extends Base, U extends Derived>((T) => U) => T");
AssertType(<T extends Base, U extends Derived>(x: (arg: T) => U) => <T>null, "<T extends Base, U extends Derived>((T) => U) => T");
AssertType(x, "(T) => U");
AssertType(arg, "T");
AssertType(<T>null, "T");
AssertType(null, "null");

let r6arg2 = (x: (arg: Base) => Derived) => <Base>null;
AssertType(r6arg2, "((Base) => Derived) => Base");
AssertType((x: (arg: Base) => Derived) => <Base>null, "((Base) => Derived) => Base");
AssertType(x, "(Base) => Derived");
AssertType(arg, "Base");
AssertType(<Base>null, "Base");
AssertType(null, "null");

let r6 = foo6(r6arg1); // any
AssertType(r6, "((Base) => Derived) => Base");
AssertType(foo6(r6arg1), "((Base) => Derived) => Base");
AssertType(foo6, "{ (((Base) => Derived) => Base): ((Base) => Derived) => Base; (any): any; }");
AssertType(r6arg1, "<T extends Base, U extends Derived>((T) => U) => T");

let r6a = [r6arg1, r6arg2];
AssertType(r6a, "(((Base) => Derived) => Base)[]");
AssertType([r6arg1, r6arg2], "(((Base) => Derived) => Base)[]");
AssertType(r6arg1, "<T extends Base, U extends Derived>((T) => U) => T");
AssertType(r6arg2, "((Base) => Derived) => Base");

let r6b = [r6arg2, r6arg1];
AssertType(r6b, "(((Base) => Derived) => Base)[]");
AssertType([r6arg2, r6arg1], "(((Base) => Derived) => Base)[]");
AssertType(r6arg2, "((Base) => Derived) => Base");
AssertType(r6arg1, "<T extends Base, U extends Derived>((T) => U) => T");

let r7arg1 = <T extends Base, U extends Derived>(x: (arg: T) => U) => (r: T) => <U>null;
AssertType(r7arg1, "<T extends Base, U extends Derived>((T) => U) => (T) => U");
AssertType(<T extends Base, U extends Derived>(x: (arg: T) => U) => (r: T) => <U>null, "<T extends Base, U extends Derived>((T) => U) => (T) => U");
AssertType(x, "(T) => U");
AssertType(arg, "T");
AssertType((r: T) => <U>null, "(T) => U");
AssertType(r, "T");
AssertType(<U>null, "U");
AssertType(null, "null");

let r7arg2 = (x: (arg: Base) => Derived) => (r: Base) => <Derived>null;
AssertType(r7arg2, "((Base) => Derived) => (Base) => Derived");
AssertType((x: (arg: Base) => Derived) => (r: Base) => <Derived>null, "((Base) => Derived) => (Base) => Derived");
AssertType(x, "(Base) => Derived");
AssertType(arg, "Base");
AssertType((r: Base) => <Derived>null, "(Base) => Derived");
AssertType(r, "Base");
AssertType(<Derived>null, "Derived");
AssertType(null, "null");

let r7 = foo7(r7arg1); // any
AssertType(r7, "((Base) => Derived) => (Base) => Derived");
AssertType(foo7(r7arg1), "((Base) => Derived) => (Base) => Derived");
AssertType(foo7, "{ (((Base) => Derived) => (Base) => Derived): ((Base) => Derived) => (Base) => Derived; (any): any; }");
AssertType(r7arg1, "<T extends Base, U extends Derived>((T) => U) => (T) => U");

let r7a = [r7arg1, r7arg2];
AssertType(r7a, "(((Base) => Derived) => (Base) => Derived)[]");
AssertType([r7arg1, r7arg2], "(((Base) => Derived) => (Base) => Derived)[]");
AssertType(r7arg1, "<T extends Base, U extends Derived>((T) => U) => (T) => U");
AssertType(r7arg2, "((Base) => Derived) => (Base) => Derived");

let r7b = [r7arg2, r7arg1];
AssertType(r7b, "(((Base) => Derived) => (Base) => Derived)[]");
AssertType([r7arg2, r7arg1], "(((Base) => Derived) => (Base) => Derived)[]");
AssertType(r7arg2, "((Base) => Derived) => (Base) => Derived");
AssertType(r7arg1, "<T extends Base, U extends Derived>((T) => U) => (T) => U");

let r8arg1 = <T extends Base, U extends Derived>(x: (arg: T) => U, y: (arg2: T) => U) => (r: T) => <U>null;
AssertType(r8arg1, "<T extends Base, U extends Derived>((T) => U, (T) => U) => (T) => U");
AssertType(<T extends Base, U extends Derived>(x: (arg: T) => U, y: (arg2: T) => U) => (r: T) => <U>null, "<T extends Base, U extends Derived>((T) => U, (T) => U) => (T) => U");
AssertType(x, "(T) => U");
AssertType(arg, "T");
AssertType(y, "(T) => U");
AssertType(arg2, "T");
AssertType((r: T) => <U>null, "(T) => U");
AssertType(r, "T");
AssertType(<U>null, "U");
AssertType(null, "null");

let r8arg2 = (x: (arg: Base) => Derived, y: (arg2: Base) => Derived) => (r: Base) => <Derived>null;
AssertType(r8arg2, "((Base) => Derived, (Base) => Derived) => (Base) => Derived");
AssertType((x: (arg: Base) => Derived, y: (arg2: Base) => Derived) => (r: Base) => <Derived>null, "((Base) => Derived, (Base) => Derived) => (Base) => Derived");
AssertType(x, "(Base) => Derived");
AssertType(arg, "Base");
AssertType(y, "(Base) => Derived");
AssertType(arg2, "Base");
AssertType((r: Base) => <Derived>null, "(Base) => Derived");
AssertType(r, "Base");
AssertType(<Derived>null, "Derived");
AssertType(null, "null");

let r8 = foo8(r8arg1); // any
AssertType(r8, "((Base) => Derived, (Base) => Derived) => (Base) => Derived");
AssertType(foo8(r8arg1), "((Base) => Derived, (Base) => Derived) => (Base) => Derived");
AssertType(foo8, "{ (((Base) => Derived, (Base) => Derived) => (Base) => Derived): ((Base) => Derived, (Base) => Derived) => (Base) => Derived; (any): any; }");
AssertType(r8arg1, "<T extends Base, U extends Derived>((T) => U, (T) => U) => (T) => U");

let r8a = [r8arg1, r8arg2];
AssertType(r8a, "(((Base) => Derived, (Base) => Derived) => (Base) => Derived)[]");
AssertType([r8arg1, r8arg2], "(((Base) => Derived, (Base) => Derived) => (Base) => Derived)[]");
AssertType(r8arg1, "<T extends Base, U extends Derived>((T) => U, (T) => U) => (T) => U");
AssertType(r8arg2, "((Base) => Derived, (Base) => Derived) => (Base) => Derived");

let r8b = [r8arg2, r8arg1];
AssertType(r8b, "(((Base) => Derived, (Base) => Derived) => (Base) => Derived)[]");
AssertType([r8arg2, r8arg1], "(((Base) => Derived, (Base) => Derived) => (Base) => Derived)[]");
AssertType(r8arg2, "((Base) => Derived, (Base) => Derived) => (Base) => Derived");
AssertType(r8arg1, "<T extends Base, U extends Derived>((T) => U, (T) => U) => (T) => U");

let r9arg1 = <T extends Base, U extends Derived>(x: (arg: T) => U, y: (arg2: { foo: string; bing: number }) => U) => (r: T) => <U>null;
AssertType(r9arg1, "<T extends Base, U extends Derived>((T) => U, ({    foo: string;    bing: number;}) => U) => (T) => U");
AssertType(<T extends Base, U extends Derived>(x: (arg: T) => U, y: (arg2: { foo: string; bing: number }) => U) => (r: T) => <U>null, "<T extends Base, U extends Derived>((T) => U, ({    foo: string;    bing: number;}) => U) => (T) => U");
AssertType(x, "(T) => U");
AssertType(arg, "T");
AssertType(y, "({    foo: string;    bing: number;}) => U");
AssertType(arg2, "{ foo: string; bing: number; }");
AssertType(foo, "string");
AssertType(bing, "number");
AssertType((r: T) => <U>null, "(T) => U");
AssertType(r, "T");
AssertType(<U>null, "U");
AssertType(null, "null");

let r9arg2 = (x: (arg: Base) => Derived, y: (arg2: Base) => Derived) => (r: Base) => <Derived>null;
AssertType(r9arg2, "((Base) => Derived, (Base) => Derived) => (Base) => Derived");
AssertType((x: (arg: Base) => Derived, y: (arg2: Base) => Derived) => (r: Base) => <Derived>null, "((Base) => Derived, (Base) => Derived) => (Base) => Derived");
AssertType(x, "(Base) => Derived");
AssertType(arg, "Base");
AssertType(y, "(Base) => Derived");
AssertType(arg2, "Base");
AssertType((r: Base) => <Derived>null, "(Base) => Derived");
AssertType(r, "Base");
AssertType(<Derived>null, "Derived");
AssertType(null, "null");

let r9 = foo9(r9arg1); // any
AssertType(r9, "((Base) => Derived, (Base) => Derived) => (Base) => Derived");
AssertType(foo9(r9arg1), "((Base) => Derived, (Base) => Derived) => (Base) => Derived");
AssertType(foo9, "{ (((Base) => Derived, (Base) => Derived) => (Base) => Derived): ((Base) => Derived, (Base) => Derived) => (Base) => Derived; (any): any; }");
AssertType(r9arg1, "<T extends Base, U extends Derived>((T) => U, ({ foo: string; bing: number; }) => U) => (T) => U");

let r9a = [r9arg1, r9arg2];
AssertType(r9a, "(((Base) => Derived, (Base) => Derived) => (Base) => Derived)[]");
AssertType([r9arg1, r9arg2], "(((Base) => Derived, (Base) => Derived) => (Base) => Derived)[]");
AssertType(r9arg1, "<T extends Base, U extends Derived>((T) => U, ({ foo: string; bing: number; }) => U) => (T) => U");
AssertType(r9arg2, "((Base) => Derived, (Base) => Derived) => (Base) => Derived");

let r9b = [r9arg2, r9arg1];
AssertType(r9b, "(((Base) => Derived, (Base) => Derived) => (Base) => Derived)[]");
AssertType([r9arg2, r9arg1], "(((Base) => Derived, (Base) => Derived) => (Base) => Derived)[]");
AssertType(r9arg2, "((Base) => Derived, (Base) => Derived) => (Base) => Derived");
AssertType(r9arg1, "<T extends Base, U extends Derived>((T) => U, ({ foo: string; bing: number; }) => U) => (T) => U");

let r10arg1 = <T extends Derived>(...x: T[]) => x[0];
AssertType(r10arg1, "<T extends Derived>(...T[]) => T");
AssertType(<T extends Derived>(...x: T[]) => x[0], "<T extends Derived>(...T[]) => T");
AssertType(x, "T[]");
AssertType(x[0], "T");
AssertType(x, "T[]");
AssertType(0, "int");

let r10arg2 = (...x: Derived[]) => <Derived>null;
AssertType(r10arg2, "(...Derived[]) => Derived");
AssertType((...x: Derived[]) => <Derived>null, "(...Derived[]) => Derived");
AssertType(x, "Derived[]");
AssertType(<Derived>null, "Derived");
AssertType(null, "null");

let r10 = foo10(r10arg1); // any
AssertType(r10, "(...Derived[]) => Derived");
AssertType(foo10(r10arg1), "(...Derived[]) => Derived");
AssertType(foo10, "{ ((...Derived[]) => Derived): (...Derived[]) => Derived; (any): any; }");
AssertType(r10arg1, "<T extends Derived>(...T[]) => T");

let r10a = [r10arg1, r10arg2];
AssertType(r10a, "((...Derived[]) => Derived)[]");
AssertType([r10arg1, r10arg2], "((...Derived[]) => Derived)[]");
AssertType(r10arg1, "<T extends Derived>(...T[]) => T");
AssertType(r10arg2, "(...Derived[]) => Derived");

let r10b = [r10arg2, r10arg1];
AssertType(r10b, "((...Derived[]) => Derived)[]");
AssertType([r10arg2, r10arg1], "((...Derived[]) => Derived)[]");
AssertType(r10arg2, "(...Derived[]) => Derived");
AssertType(r10arg1, "<T extends Derived>(...T[]) => T");

let r11arg1 = <T extends Base>(x: T, y: T) => x;
AssertType(r11arg1, "<T extends Base>(T, T) => T");
AssertType(<T extends Base>(x: T, y: T) => x, "<T extends Base>(T, T) => T");
AssertType(x, "T");
AssertType(y, "T");
AssertType(x, "T");

let r11arg2 = (x: { foo: string }, y: { foo: string; bar: string }) => <Base>null;
AssertType(r11arg2, "({    foo: string;}, {    foo: string;    bar: string;}) => Base");
AssertType((x: { foo: string }, y: { foo: string; bar: string }) => <Base>null, "({    foo: string;}, {    foo: string;    bar: string;}) => Base");
AssertType(x, "{ foo: string; }");
AssertType(foo, "string");
AssertType(y, "{ foo: string; bar: string; }");
AssertType(foo, "string");
AssertType(bar, "string");
AssertType(<Base>null, "Base");
AssertType(null, "null");

let r11 = foo11(r11arg1); // any
AssertType(r11, "({ foo: string; }, { foo: string; bar: string; }) => Base");
AssertType(foo11(r11arg1), "({ foo: string; }, { foo: string; bar: string; }) => Base");
AssertType(foo11, "{ (({ foo: string; }, { foo: string; bar: string; }) => Base): ({ foo: string; }, { foo: string; bar: string; }) => Base; (any): any; }");
AssertType(r11arg1, "<T extends Base>(T, T) => T");

let r11a = [r11arg1, r11arg2];
AssertType(r11a, "(({ foo: string; }, { foo: string; bar: string; }) => Base)[]");
AssertType([r11arg1, r11arg2], "(({ foo: string; }, { foo: string; bar: string; }) => Base)[]");
AssertType(r11arg1, "<T extends Base>(T, T) => T");
AssertType(r11arg2, "({ foo: string; }, { foo: string; bar: string; }) => Base");

let r11b = [r11arg2, r11arg1];
AssertType(r11b, "(({ foo: string; }, { foo: string; bar: string; }) => Base)[]");
AssertType([r11arg2, r11arg1], "(({ foo: string; }, { foo: string; bar: string; }) => Base)[]");
AssertType(r11arg2, "({ foo: string; }, { foo: string; bar: string; }) => Base");
AssertType(r11arg1, "<T extends Base>(T, T) => T");

let r12arg1 = <T extends Array<Base>>(x: Array<Base>, y: T) => <Array<Derived>>null;
AssertType(r12arg1, "<T extends Base[]>(Array<Base>, T) => Derived[]");
AssertType(<T extends Array<Base>>(x: Array<Base>, y: T) => <Array<Derived>>null, "<T extends Base[]>(Array<Base>, T) => Derived[]");
AssertType(x, "Base[]");
AssertType(y, "T");
AssertType(<Array<Derived>>null, "Derived[]");
AssertType(null, "null");

let r12arg2 = (x: Array<Base>, y: Array<Derived2>) => <Array<Derived>>null;
AssertType(r12arg2, "(Array<Base>, Array<Derived2>) => Derived[]");
AssertType((x: Array<Base>, y: Array<Derived2>) => <Array<Derived>>null, "(Array<Base>, Array<Derived2>) => Derived[]");
AssertType(x, "Base[]");
AssertType(y, "Derived2[]");
AssertType(<Array<Derived>>null, "Derived[]");
AssertType(null, "null");

let r12 = foo12(r12arg1); // any
AssertType(r12, "(Base[], Derived2[]) => Derived[]");
AssertType(foo12(r12arg1), "(Base[], Derived2[]) => Derived[]");
AssertType(foo12, "{ ((Base[], Derived2[]) => Derived[]): (Base[], Derived2[]) => Derived[]; (any): any; }");
AssertType(r12arg1, "<T extends Base[]>(Base[], T) => Derived[]");

let r12a = [r12arg1, r12arg2];
AssertType(r12a, "((Base[], Derived2[]) => Derived[])[]");
AssertType([r12arg1, r12arg2], "((Base[], Derived2[]) => Derived[])[]");
AssertType(r12arg1, "<T extends Base[]>(Base[], T) => Derived[]");
AssertType(r12arg2, "(Base[], Derived2[]) => Derived[]");

let r12b = [r12arg2, r12arg1];
AssertType(r12b, "((Base[], Derived2[]) => Derived[])[]");
AssertType([r12arg2, r12arg1], "((Base[], Derived2[]) => Derived[])[]");
AssertType(r12arg2, "(Base[], Derived2[]) => Derived[]");
AssertType(r12arg1, "<T extends Base[]>(Base[], T) => Derived[]");

let r13arg1 = <T extends Array<Derived>>(x: Array<Base>, y: T) => y;
AssertType(r13arg1, "<T extends Derived[]>(Array<Base>, T) => T");
AssertType(<T extends Array<Derived>>(x: Array<Base>, y: T) => y, "<T extends Derived[]>(Array<Base>, T) => T");
AssertType(x, "Base[]");
AssertType(y, "T");
AssertType(y, "T");

let r13arg2 = (x: Array<Base>, y: Array<Derived>) => <Array<Derived>>null;
AssertType(r13arg2, "(Array<Base>, Array<Derived>) => Derived[]");
AssertType((x: Array<Base>, y: Array<Derived>) => <Array<Derived>>null, "(Array<Base>, Array<Derived>) => Derived[]");
AssertType(x, "Base[]");
AssertType(y, "Derived[]");
AssertType(<Array<Derived>>null, "Derived[]");
AssertType(null, "null");

let r13 = foo13(r13arg1); // any
AssertType(r13, "(Base[], Derived[]) => Derived[]");
AssertType(foo13(r13arg1), "(Base[], Derived[]) => Derived[]");
AssertType(foo13, "{ ((Base[], Derived[]) => Derived[]): (Base[], Derived[]) => Derived[]; (any): any; }");
AssertType(r13arg1, "<T extends Derived[]>(Base[], T) => T");

let r13a = [r13arg1, r13arg2];
AssertType(r13a, "((Base[], Derived[]) => Derived[])[]");
AssertType([r13arg1, r13arg2], "((Base[], Derived[]) => Derived[])[]");
AssertType(r13arg1, "<T extends Derived[]>(Base[], T) => T");
AssertType(r13arg2, "(Base[], Derived[]) => Derived[]");

let r13b = [r13arg2, r13arg1];
AssertType(r13b, "((Base[], Derived[]) => Derived[])[]");
AssertType([r13arg2, r13arg1], "((Base[], Derived[]) => Derived[])[]");
AssertType(r13arg2, "(Base[], Derived[]) => Derived[]");
AssertType(r13arg1, "<T extends Derived[]>(Base[], T) => T");

let r14arg1 = <T>(x: { a: T; b: T }) => x.a;
AssertType(r14arg1, "<T>({ a: T; b: T; }) => T");
AssertType(<T>(x: { a: T; b: T }) => x.a, "<T>({ a: T; b: T; }) => T");
AssertType(x, "{ a: T; b: T; }");
AssertType(a, "T");
AssertType(b, "T");
AssertType(x.a, "T");

let r14arg2 = (x: { a: string; b: number }) => <Object>null;
AssertType(r14arg2, "({    a: string;    b: number;}) => Object");
AssertType((x: { a: string; b: number }) => <Object>null, "({    a: string;    b: number;}) => Object");
AssertType(x, "{ a: string; b: number; }");
AssertType(a, "string");
AssertType(b, "number");
AssertType(<Object>null, "Object");
AssertType(null, "null");

let r14 = foo14(r14arg1); // any
AssertType(r14, "any");
AssertType(foo14(r14arg1), "any");
AssertType(foo14, "{ (({ a: string; b: number; }) => Object): ({ a: string; b: number; }) => Object; (any): any; }");
AssertType(r14arg1, "<T>({ a: T; b: T; }) => T");

let r14a = [r14arg1, r14arg2];
AssertType(r14a, "(union)[]");
AssertType([r14arg1, r14arg2], "(union)[]");
AssertType(r14arg1, "<T>({ a: T; b: T; }) => T");
AssertType(r14arg2, "({ a: string; b: number; }) => Object");

let r14b = [r14arg2, r14arg1];
AssertType(r14b, "(union)[]");
AssertType([r14arg2, r14arg1], "(union)[]");
AssertType(r14arg2, "({ a: string; b: number; }) => Object");
AssertType(r14arg1, "<T>({ a: T; b: T; }) => T");

let r15arg1 = <T>(x: T) => <T[]>null
AssertType(r15arg1, "<T>(T) => T[]");
AssertType(<T>(x: T) => <T[]>null, "<T>(T) => T[]");
AssertType(x, "T");
AssertType(<T[]>null, "T[]");
AssertType(null, "null");

let r15 = foo15(r15arg1); // any
AssertType(r15, "any");
AssertType(foo15(r15arg1), "any");
AssertType(foo15, "{ ({ (number): number[]; (string): string[]; }): { (number): number[]; (string): string[]; }; (any): any; }");
AssertType(r15arg1, "<T>(T) => T[]");

let r16arg1 = <T extends Base>(x: T) => [1];
AssertType(r16arg1, "<T extends Base>(T) => number[]");
AssertType(<T extends Base>(x: T) => [1], "<T extends Base>(T) => number[]");
AssertType(x, "T");
AssertType([1], "number[]");
AssertType(1, "int");

let r16 = foo16(r16arg1); 
AssertType(r16, "{ <T extends Derived>(T): number[]; <U extends Base>(U): number[]; }");
AssertType(foo16(r16arg1), "{ <T extends Derived>(T): number[]; <U extends Base>(U): number[]; }");
AssertType(foo16, "{ ({ <T extends Derived>(T): number[]; <U extends Base>(U): number[]; }): { <T extends Derived>(T): number[]; <U extends Base>(U): number[]; }; (any): any; }");
AssertType(r16arg1, "<T extends Base>(T) => number[]");

let r17arg1 = <T>(x: (a: T) => T) => <T[]>null;
AssertType(r17arg1, "<T>((T) => T) => T[]");
AssertType(<T>(x: (a: T) => T) => <T[]>null, "<T>((T) => T) => T[]");
AssertType(x, "(T) => T");
AssertType(a, "T");
AssertType(<T[]>null, "T[]");
AssertType(null, "null");

let r17 = foo17(r17arg1); // any
AssertType(r17, "any");
AssertType(foo17(r17arg1), "any");
AssertType(foo17, "{ ({ ((number) => number): number[]; ((string) => string): string[]; }): { ((number) => number): number[]; ((string) => string): string[]; }; (any): any; }");
AssertType(r17arg1, "<T>((T) => T) => T[]");

let r18arg1 = <T>(x: (a: T) => T) => <T[]>null;
AssertType(r18arg1, "<T>((T) => T) => T[]");
AssertType(<T>(x: (a: T) => T) => <T[]>null, "<T>((T) => T) => T[]");
AssertType(x, "(T) => T");
AssertType(a, "T");
AssertType(<T[]>null, "T[]");
AssertType(null, "null");

let r18 = foo18(r18arg1); 
AssertType(r18, "{ ({ (number): number; (string): string; }): any[]; ({ (boolean): boolean; (Date): Date; }): any[]; }");
AssertType(foo18(r18arg1), "{ ({ (number): number; (string): string; }): any[]; ({ (boolean): boolean; (Date): Date; }): any[]; }");
AssertType(foo18, "{ ({ ({ (number): number; (string): string; }): any[]; ({ (boolean): boolean; (Date): Date; }): any[]; }): { ({ (number): number; (string): string; }): any[]; ({ (boolean): boolean; (Date): Date; }): any[]; }; (any): any; }");
AssertType(r18arg1, "<T>((T) => T) => T[]");


