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

// === tests/cases/conformance/internalModules/importDeclarations/exportImportAlias.ts ===
declare function AssertType(value:any, type:string):void;
// expect no errors here

module A {

    export let x = 'hello world'
    export class Point {
        constructor(public x: number, public y: number) { }
    }
    export module B {
        export interface Id {
            name: string;
        }
    }
}

module C {
    export import a = A;
}

let a: string = C.a.x;
AssertType(a, "string");
AssertType(C.a.x, "string");

let b: { x: number; y: number; } = new C.a.Point(0, 0);
AssertType(b, "{ x: number; y: number; }");
AssertType(x, "number");
AssertType(y, "number");
AssertType(new C.a.Point(0, 0), "A.Point");
AssertType(C.a.Point, "typeof A.Point");
AssertType(0, "int");
AssertType(0, "int");

let c: { name: string };
AssertType(c, "{ name: string; }");
AssertType(name, "string");

let c: C.a.B.Id;
AssertType(c, "{ name: string; }");
AssertType(C, "any");
AssertType(a, "any");
AssertType(B, "any");

module X {
    export function Y() {
AssertType(42, "int");
        return 42;
    }

    export module Y {
        export class Point {
            constructor(public x: number, public y: number) { }
        }
    }
}

module Z {

    // 'y' should be a fundule here
    export import y = X.Y;
}

let m: number = Z.y();
AssertType(m, "number");
AssertType(Z.y(), "number");
AssertType(Z.y, "typeof X.Y");

let n: { x: number; y: number; } = new Z.y.Point(0, 0);
AssertType(n, "{ x: number; y: number; }");
AssertType(x, "number");
AssertType(y, "number");
AssertType(new Z.y.Point(0, 0), "X.Y.Point");
AssertType(Z.y.Point, "typeof X.Y.Point");
AssertType(0, "int");
AssertType(0, "int");

module K {
    export class L {
        constructor(public name: string) { }
    }

    export module L {
        export let y = 12;
        export interface Point {
            x: number;
            y: number;
        }
    }
}

module M {
    export import D = K.L;
}

let o: { name: string };
AssertType(o, "{ name: string; }");
AssertType(name, "string");

let o = new M.D('Hello');
AssertType(o, "{ name: string; }");
AssertType(new M.D('Hello'), "K.L");
AssertType(M.D, "typeof K.L");
AssertType('Hello', "string");

let p: { x: number; y: number; 
AssertType(p, "{ x: number; y: number; }");

AssertType(x, "number");

AssertType(y, "number");
}

let p: M.D.Point;
AssertType(p, "{ x: number; y: number; }");
AssertType(M, "any");
AssertType(D, "any");


