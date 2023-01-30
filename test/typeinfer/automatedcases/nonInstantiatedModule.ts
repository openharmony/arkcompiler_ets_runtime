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

// === tests/cases/conformance/internalModules/moduleDeclarations/nonInstantiatedModule.ts ===
declare function AssertType(value:any, type:string):void;
module M {
    export interface Point { x: number; y: number }
    export let a = 1;
}

// primary expression
let m : typeof M;
AssertType(m, "typeof M");
AssertType(M, "typeof M");

let m = M;
AssertType(m, "typeof M");
AssertType(M, "typeof M");

let a1: number;
AssertType(a1, "number");

let a1 = M.a;
AssertType(a1, "number");
AssertType(M.a, "number");

let a2: number;
AssertType(a2, "number");

let a2 = m.a;
AssertType(a2, "number");
AssertType(m.a, "number");

module M2 {
    export module Point {
        export function Origin(): Point {
AssertType({ x: 0, y: 0 }, "{ x: number; y: number; }");
AssertType(x, "number");
AssertType(0, "int");
AssertType(y, "number");
AssertType(0, "int");
            return { x: 0, y: 0 };
        }
    }

    export interface Point {
        x: number;
        y: number;
    }
}

let p: { x: number; y: number; };
AssertType(p, "{ x: number; y: number; }");
AssertType(x, "number");
AssertType(y, "number");

let p: M2.Point;
AssertType(p, "{ x: number; y: number; }");
AssertType(M2, "any");

let p2: { Origin() : { x: number; y: number; } };
AssertType(p2, "{ Origin(): {    x: number;    y: number;}; }");
AssertType(Origin, "() => {    x: number;    y: number;}");
AssertType(x, "number");
AssertType(y, "number");

let p2: typeof M2.Point;
AssertType(p2, "{ Origin(): { x: number; y: number; }; }");
AssertType(M2.Point, "typeof M2.Point");
AssertType(M2, "typeof M2");
AssertType(Point, "typeof M2.Point");

module M3 {
    export module Utils {
        export interface Point {
            x: number; y: number;
        }
    }

    export class Utils {
        name: string;
    }
}

