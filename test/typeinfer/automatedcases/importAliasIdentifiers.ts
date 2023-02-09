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

// === tests/cases/conformance/internalModules/importDeclarations/importAliasIdentifiers.ts ===
declare function AssertType(value:any, type:string):void;
module moduleA {
    export class Point {
        constructor(public x: number, public y: number) { }
    }
}

import alias = moduleA;

let p: alias.Point;
AssertType(p, "alias.Point");
AssertType(alias, "any");

let p: moduleA.Point;
AssertType(p, "alias.Point");
AssertType(moduleA, "any");

let p: { x: number; y: number; };
AssertType(p, "alias.Point");
AssertType(x, "number");
AssertType(y, "number");

class clodule {
    name: string;
}

module clodule {
    export interface Point {
        x: number; y: number;
    }
    let Point: Point = { x: 0, y: 0 };
}

import clolias = clodule;

let p: clolias.Point;
AssertType(p, "alias.Point");
AssertType(clolias, "any");

let p: clodule.Point;
AssertType(p, "alias.Point");
AssertType(clodule, "any");

let p: { x: number; y: number; };
AssertType(p, "alias.Point");
AssertType(x, "number");
AssertType(y, "number");


function fundule() {
AssertType({ x: 0, y: 0 }, "{ x: number; y: number; }");
AssertType(x, "number");
AssertType(0, "int");
AssertType(y, "number");
AssertType(0, "int");
    return { x: 0, y: 0 };
}

module fundule {
    export interface Point {
        x: number; y: number;
    }
    let Point: Point = { x: 0, y: 0 };
}

import funlias = fundule;

let p: funlias.Point;
AssertType(p, "alias.Point");
AssertType(funlias, "any");

let p: fundule.Point;
AssertType(p, "alias.Point");
AssertType(fundule, "any");

let p: { x: number; y: number; };
AssertType(p, "alias.Point");
AssertType(x, "number");
AssertType(y, "number");


