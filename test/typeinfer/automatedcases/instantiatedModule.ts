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

// === tests/cases/conformance/internalModules/moduleDeclarations/instantiatedModule.ts ===
declare function AssertType(value:any, type:string):void;
// adding the let makes this an instantiated module

module M {
    export interface Point { x: number; y: number }
    export let Point = 1;
}

// primary expression
let m: typeof M;
AssertType(m, "typeof M");
AssertType(M, "typeof M");

let m = M;
AssertType(m, "typeof M");
AssertType(M, "typeof M");

let a1: number;
AssertType(a1, "number");

let a1 = M.Point;
AssertType(a1, "number");
AssertType(M.Point, "number");

let a1 = m.Point;
AssertType(a1, "number");
AssertType(m.Point, "number");

let p1: { x: number; y: number; 
AssertType(p1, "{ x: number; y: number; }");

AssertType(x, "number");

AssertType(y, "number");
}

let p1: M.Point;
AssertType(p1, "{ x: number; y: number; }");
AssertType(M, "any");

// making the point a class instead of an interface 
// makes this an instantiated mmodule
module M2 {
    export class Point {
        x: number;
        y: number;
        static Origin(): Point {
AssertType({ x: 0, y: 0 }, "{ x: number; y: number; }");
AssertType(x, "number");
AssertType(0, "int");
AssertType(y, "number");
AssertType(0, "int");
            return { x: 0, y: 0 };
        }
    }
}

let m2: typeof M2;
AssertType(m2, "typeof M2");
AssertType(M2, "typeof M2");

let m2 = M2;
AssertType(m2, "typeof M2");
AssertType(M2, "typeof M2");

// static side of the class
let a2: typeof M2.Point;
AssertType(a2, "typeof M2.Point");
AssertType(M2.Point, "typeof M2.Point");
AssertType(M2, "typeof M2");
AssertType(Point, "typeof M2.Point");

let a2 = m2.Point;
AssertType(a2, "typeof M2.Point");
AssertType(m2.Point, "typeof M2.Point");

let a2 = M2.Point;
AssertType(a2, "typeof M2.Point");
AssertType(M2.Point, "typeof M2.Point");

let o: M2.Point = a2.Origin();
AssertType(o, "M2.Point");
AssertType(M2, "any");
AssertType(a2.Origin(), "M2.Point");
AssertType(a2.Origin, "() => M2.Point");

let p2: { x: number; y: number 
AssertType(p2, "{ x: number; y: number; }");

AssertType(x, "number");

AssertType(y, "number");
}

let p2: M2.Point;
AssertType(p2, "{ x: number; y: number; }");
AssertType(M2, "any");

let p2 = new m2.Point();
AssertType(p2, "{ x: number; y: number; }");
AssertType(new m2.Point(), "M2.Point");
AssertType(m2.Point, "typeof M2.Point");

let p2 = new M2.Point();
AssertType(p2, "{ x: number; y: number; }");
AssertType(new M2.Point(), "M2.Point");
AssertType(M2.Point, "typeof M2.Point");

module M3 {
    export enum Color { Blue, Red }
}

let m3: typeof M3;
AssertType(m3, "typeof M3");
AssertType(M3, "typeof M3");

let m3 = M3;
AssertType(m3, "typeof M3");
AssertType(M3, "typeof M3");

let a3: typeof M3.Color;
AssertType(a3, "typeof M3.Color");
AssertType(M3.Color, "typeof M3.Color");
AssertType(M3, "typeof M3");
AssertType(Color, "typeof M3.Color");

let a3 = m3.Color;
AssertType(a3, "typeof M3.Color");
AssertType(m3.Color, "typeof M3.Color");

let a3 = M3.Color;
AssertType(a3, "typeof M3.Color");
AssertType(M3.Color, "typeof M3.Color");

let blue: M3.Color = a3.Blue;
AssertType(blue, "M3.Color");
AssertType(M3, "any");
AssertType(a3.Blue, "M3.Color.Blue");

let p3: M3.Color;
AssertType(p3, "M3.Color");
AssertType(M3, "any");

let p3 = M3.Color.Red;
AssertType(p3, "M3.Color");
AssertType(M3.Color.Red, "M3.Color.Red");

let p3 = m3.Color.Blue;
AssertType(p3, "M3.Color");
AssertType(m3.Color.Blue, "M3.Color.Blue");


