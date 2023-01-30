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

// === tests/cases/compiler/letdecl.ts ===
declare function AssertType(value:any, type:string):void;
let simpleVar;
AssertType(simpleVar, "any");

let anotherVar: any;
AssertType(anotherVar, "any");

let letWithSimpleType: number;
AssertType(letWithSimpleType, "number");

let letWithArrayType: number[];
AssertType(letWithArrayType, "number[]");

let letWithInitialValue = 30;
AssertType(letWithInitialValue, "number");
AssertType(30, "int");

let withComplicatedValue = { x: 30, y: 70, desc: "position" };
AssertType(withComplicatedValue, "{ x: number; y: number; desc: string; }");
AssertType({ x: 30, y: 70, desc: "position" }, "{ x: number; y: number; desc: string; }");
AssertType(x, "number");
AssertType(30, "int");
AssertType(y, "number");
AssertType(70, "int");
AssertType(desc, "string");
AssertType("position", "string");

declare let declaredVar;
AssertType(declaredVar, "any");

declare let declareVar2
AssertType(declareVar2, "any");

declare let declaredVar3;
AssertType(declaredVar3, "any");

declare let deckareVarWithType: number;
AssertType(deckareVarWithType, "number");

let arrayVar: string[] = ['a', 'b'];
AssertType(arrayVar, "string[]");
AssertType(['a', 'b'], "string[]");
AssertType('a', "string");
AssertType('b', "string");

let complicatedArrayVar: { x: number; y: string; }[] ;
AssertType(complicatedArrayVar, "{ x: number; y: string; }[]");
AssertType(x, "number");
AssertType(y, "string");

complicatedArrayVar.push({ x: 30, y : 'hello world' });
AssertType(complicatedArrayVar.push({ x: 30, y : 'hello world' }), "number");
AssertType(complicatedArrayVar.push, "(...{ x: number; y: string; }[]) => number");
AssertType({ x: 30, y : 'hello world' }, "{ x: number; y: string; }");
AssertType(x, "number");
AssertType(30, "int");
AssertType(y, "string");
AssertType('hello world', "string");

let n1: { [s: string]: number; };
AssertType(n1, "{ [string]: number; }");
AssertType(s, "string");

let c : {
AssertType(c, "{ new?(): any; }");

        new? (): any;
    }

let d: {
AssertType(d, "{ foo?(): {    x: number;}; }");

    foo? (): {
AssertType(foo, "() => {    x: number;}");

        x: number;
AssertType(x, "number");

    };
}

let d3: {
AssertType(d3, "{ foo(): {    x: number;    y: number;}; }");

    foo(): {
AssertType(foo, "() => {    x: number;    y: number;}");

        x: number;
AssertType(x, "number");

        y: number;
AssertType(y, "number");

    };
}

let d2: {
AssertType(d2, "{ foo(): {    x: number;}; }");

    foo (): {
AssertType(foo, "() => {    x: number;}");

        x: number;
AssertType(x, "number");

    };
}

let n2: {
AssertType(n2, "() => void");

    (): void;
}
let n4: {
AssertType(n4, "(() => void)[]");

    (): void;
}[];

let d4: {
AssertType(d4, "{ foo(string, { x: number; y: number; }): { x: number; y: number; }; }");

    foo(n: string, x: { x: number; y: number; }): {
AssertType(foo, "(string, {    x: number;    y: number;}) => {    x: number;    y: number;}");
AssertType(n, "string");
AssertType(x, "{ x: number; y: number; }");
AssertType(x, "number");
AssertType(y, "number");

        x: number;
AssertType(x, "number");

        y: number;
AssertType(y, "number");

    };
}

module m2 {

    export let a, b2: number = 10, b;
    let m1;
    let a2, b22: number = 10, b222;
    let m3;

    class C {
        constructor (public b) {
        }
    }

    export class C2 {
        constructor (public b) {
        }
    }
    let m;
    declare let d1, d2;
    let b23;
    declare let v1;
    export let mE;
    export declare let d1E, d2E;
    export let b2E;
    export declare let v1E;
}

let a22, b22 = 10, c22 = 30;
AssertType(a22, "any");
AssertType(b22, "number");
AssertType(10, "int");
AssertType(c22, "number");
AssertType(30, "int");

let nn;
AssertType(nn, "any");

declare let da1, da2;
AssertType(da1, "any");
AssertType(da2, "any");

let normalVar;
AssertType(normalVar, "any");

declare let dv1;
AssertType(dv1, "any");

let xl;
AssertType(xl, "any");

let x;
AssertType(x, "any");

let z;
AssertType(z, "any");

function foo(a2) {
    let a = 10;
AssertType(a, "number");
AssertType(10, "int");
}

for (let i = 0, j = 0; i < 10; i++) {
    j++;
AssertType(j++, "number");
AssertType(j, "number");
}


for (let k = 0; k < 30; k++) {
    k++;
AssertType(k++, "number");
AssertType(k, "number");
}
let b = 10;
AssertType(b, "number");
AssertType(10, "int");


