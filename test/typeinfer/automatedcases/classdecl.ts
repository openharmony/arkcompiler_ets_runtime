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

// === tests/cases/compiler/classdecl.ts ===
declare function AssertType(value:any, type:string):void;
class a {
    constructor (n: number);
    constructor (s: string);
    constructor (ns: any) {

    }

    public pgF() { }

    public pv;
    public get d() {
AssertType(30, "int");
        return 30;
    }
    public set d(a: number) {
    }

    public static get p2() {
AssertType({ x: 30, y: 40 }, "{ x: number; y: number; }");
AssertType(x, "number");
AssertType(30, "int");
AssertType(y, "number");
AssertType(40, "int");
        return { x: 30, y: 40 };
    }

    private static d2() {
    }
    private static get p3() {
AssertType("string", "string");
        return "string";
    }
    private pv3;

    private foo(n: number): string;
    private foo(s: string): string;
    private foo(ns: any) {
AssertType(ns.toString(), "any");
AssertType(ns.toString, "any");
        return ns.toString();
    }
}

class b extends a {
}

module m1 {
    export class b {
    }
    class d {
    }


    export interface ib {
    }
}

module m2 {

    export module m3 {
        export class c extends b {
        }
        export class ib2 implements m1.ib {
        }
    }
}

class c extends m1.b {
}

class ib2 implements m1.ib {
}

declare class aAmbient {
    constructor (n: number);
    constructor (s: string);
    public pgF(): void;
    public pv;
    public d : number;
    static p2 : { x: number; y: number; };
    static d2();
    static p3;
    private pv3;
    private foo(s);
}

class d {
    private foo(n: number): string;
    private foo(s: string): string;
    private foo(ns: any) {
AssertType(ns.toString(), "any");
AssertType(ns.toString, "any");
        return ns.toString();
    }    
}

class e {    
    private foo(s: string): string;
    private foo(n: number): string;
    private foo(ns: any) {
AssertType(ns.toString(), "any");
AssertType(ns.toString, "any");
        return ns.toString();
    }
}

