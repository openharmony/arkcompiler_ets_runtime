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

// === tests/cases/conformance/types/typeRelationships/subtypesAndSuperTypes/subtypingWithConstructSignatures3.ts ===
declare function AssertType(value:any, type:string):void;

// checking subtype relations for function types as it relates to contextual signature instantiation
// error cases, so function calls will all result in 'any'

module Errors {
    class Base { foo: string; }
    class Derived extends Base { bar: string; }
    class Derived2 extends Derived { baz: string; }
    class OtherDerived extends Base { bing: string; }

    declare function foo2(a2: new (x: number) => string[]): typeof a2;
    declare function foo2(a2: any): any;

    declare function foo7(a2: new (x: new (arg: Base) => Derived) => new (r: Base) => Derived2): typeof a2;
    declare function foo7(a2: any): any;

    declare function foo8(a2: new (x: new (arg: Base) => Derived, y: new (arg2: Base) => Derived) => new (r: Base) => Derived): typeof a2;
    declare function foo8(a2: any): any;

    declare function foo10(a2: new (...x: Base[]) => Base): typeof a2;
    declare function foo10(a2: any): any;

    declare function foo11(a2: new (x: { foo: string }, y: { foo: string; bar: string }) => Base): typeof a2;
    declare function foo11(a2: any): any;

    declare function foo12(a2: new (x: Array<Base>, y: Array<Derived2>) => Array<Derived>): typeof a2;
    declare function foo12(a2: any): any;

    declare function foo15(a2: new (x: { a: string; b: number }) => number): typeof a2;
    declare function foo15(a2: any): any;

    declare function foo16(a2: {
        // type of parameter is overload set which means we can't do inference based on this type
        new (x: {
            new (a: number): number;
            new (a?: number): number;
        }): number[];
        new (x: {
            new (a: boolean): boolean;
            new (a?: boolean): boolean;
        }): boolean[];
    }): typeof a2;
    declare function foo16(a2: any): any;

    declare function foo17(a2: {
        new (x: {
            new <T extends Derived>(a: T): T;
            new <T extends Base>(a: T): T;
        }): any[];
        new (x: {
            new <T extends Derived2>(a: T): T;
            new <T extends Base>(a: T): T;
        }): any[];
    }): typeof a2;
    declare function foo17(a2: any): any;

    let r1arg1: new <T, U>(x: T) => U[];
    let r1arg2: new (x: number) => string[]; 
    let r1 = foo2(r1arg1); // any
    let r1a = [r1arg2, r1arg1];
    let r1b = [r1arg1, r1arg2];

    let r2arg1: new <T extends Base, U extends Derived, V extends Derived2>(x: new (arg: T) => U) => new (r: T) => V;
    let r2arg2: new (x: new (arg: Base) => Derived) => new (r: Base) => Derived2;
    let r2 = foo7(r2arg1); // any
    let r2a = [r2arg2, r2arg1];
    let r2b = [r2arg1, r2arg2];

    let r3arg1: new <T extends Base, U extends Derived>(x: new (arg: T) => U, y: (arg2: { foo: number; }) => U) => new (r: T) => U;
    let r3arg2: new (x: (arg: Base) => Derived, y: new (arg2: Base) => Derived) => new (r: Base) => Derived;
    let r3 = foo8(r3arg1); // any
    let r3a = [r3arg2, r3arg1];
    let r3b = [r3arg1, r3arg2];

    let r4arg1: new <T extends Derived>(...x: T[]) => T;
    let r4arg2: new (...x: Base[]) => Base;
    let r4 = foo10(r4arg1); // any
    let r4a = [r4arg2, r4arg1];
    let r4b = [r4arg1, r4arg2];

    let r5arg1: new <T extends Derived>(x: T, y: T) => T;
    let r5arg2: new (x: { foo: string }, y: { foo: string; bar: string }) => Base;
    let r5 = foo11(r5arg1); // any
    let r5a = [r5arg2, r5arg1];
    let r5b = [r5arg1, r5arg2];

    let r6arg1: new (x: Array<Base>, y: Array<Derived2>) => Array<Derived>;
    let r6arg2: new <T extends Array<Derived2>>(x: Array<Base>, y: Array<Base>) => T;
    let r6 = foo12(r6arg1); // new (x: Array<Base>, y: Array<Derived2>) => Array<Derived>
    let r6a = [r6arg2, r6arg1];
    let r6b = [r6arg1, r6arg2];

    let r7arg1: new <T>(x: { a: T; b: T }) => T;
    let r7arg2: new (x: { a: string; b: number }) => number;
    let r7 = foo15(r7arg1); // (x: { a: string; b: number }) => number): number;
    let r7a = [r7arg2, r7arg1];
    let r7b = [r7arg1, r7arg2];

    let r7arg3: new <T extends Base>(x: { a: T; b: T }) => number;
    let r7c = foo15(r7arg3); // any
    let r7d = [r7arg2, r7arg3];
    let r7e = [r7arg3, r7arg2];

    let r8arg: new <T>(x: new (a: T) => T) => T[];
    let r8 = foo16(r8arg); // any

    let r9arg: new <T>(x: new (a: T) => T) => any[];
    let r9 = foo17(r9arg); // // (x: { <T extends Derived >(a: T): T; <T extends Base >(a: T): T; }): any[]; (x: { <T extends Derived2>(a: T): T; <T extends Base>(a: T): T; }): any[];
}

module WithGenericSignaturesInBaseType {
    declare function foo2(a2: new <T>(x: T) => T[]): typeof a2;
    declare function foo2(a2: any): any;
    let r2arg2: new <T>(x: T) => string[];
    let r2 = foo2(r2arg2); // <T>(x:T) => T[] since we can infer from generic signatures now

    declare function foo3(a2: new <T>(x: T) => string[]): typeof a2;
    declare function foo3(a2: any): any;
    let r3arg2: new <T>(x: T) => T[];
    let r3 = foo3(r3arg2); // any
}

