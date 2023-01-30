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

// === tests/cases/compiler/withExportDecl.ts ===
declare function AssertType(value:any, type:string):void;
let simpleVar;
AssertType(simpleVar, "any");

export let exportedSimpleVar;
AssertType(exportedSimpleVar, "any");

let anotherVar: any;
AssertType(anotherVar, "any");

let letWithSimpleType: number;
AssertType(letWithSimpleType, "number");

let letWithArrayType: number[];
AssertType(letWithArrayType, "number[]");

let letWithInitialValue = 30;
AssertType(letWithInitialValue, "number");
AssertType(30, "int");

export let exportedVarWithInitialValue = 70;
AssertType(exportedVarWithInitialValue, "number");
AssertType(70, "int");

let withComplicatedValue = { x: 30, y: 70, desc: "position" };
AssertType(withComplicatedValue, "{ x: number; y: number; desc: string; }");
AssertType({ x: 30, y: 70, desc: "position" }, "{ x: number; y: number; desc: string; }");
AssertType(x, "number");
AssertType(30, "int");
AssertType(y, "number");
AssertType(70, "int");
AssertType(desc, "string");
AssertType("position", "string");

export let exportedWithComplicatedValue = { x: 30, y: 70, desc: "position" };
AssertType(exportedWithComplicatedValue, "{ x: number; y: number; desc: string; }");
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

declare let declaredVar;
AssertType(declaredVar, "any");

declare let deckareVarWithType: number;
AssertType(deckareVarWithType, "number");

export declare let exportedDeclaredVar: number;
AssertType(exportedDeclaredVar, "number");

let arrayVar: string[] = ['a', 'b'];
AssertType(arrayVar, "string[]");
AssertType(['a', 'b'], "string[]");
AssertType('a', "string");
AssertType('b', "string");

export let exportedArrayVar: { x: number; y: string; }[] ;
AssertType(exportedArrayVar, "{ x: number; y: string; }[]");
AssertType(x, "number");
AssertType(y, "string");

exportedArrayVar.push({ x: 30, y : 'hello world' });
AssertType(exportedArrayVar.push({ x: 30, y : 'hello world' }), "number");
AssertType(exportedArrayVar.push, "(...{ x: number; y: string; }[]) => number");
AssertType({ x: 30, y : 'hello world' }, "{ x: number; y: string; }");
AssertType(x, "number");
AssertType(30, "int");
AssertType(y, "string");
AssertType('hello world', "string");

function simpleFunction() {
AssertType({        x: "Hello",        y: "word",        n: 2    }, "{ x: string; y: string; n: number; }");
    return {

        x: "Hello",
AssertType(x, "string");
AssertType("Hello", "string");

        y: "word",
AssertType(y, "string");
AssertType("word", "string");

        n: 2
AssertType(n, "number");
AssertType(2, "int");

    };
}

export function exportedFunction() {
AssertType(simpleFunction(), "{ x: string; y: string; n: number; }");
AssertType(simpleFunction, "() => { x: string; y: string; n: number; }");
    return simpleFunction();
}

module m1 {
    export function foo() {
AssertType("Hello", "string");
        return "Hello";
    }
}
export declare module m2 {

    export let a: number;
}


export module m3 {

    export function foo() {
AssertType(m1.foo(), "string");
AssertType(m1.foo, "() => string");
        return m1.foo();
    }
}

export let eVar1, eVar2 = 10;
AssertType(eVar1, "any");
AssertType(eVar2, "number");
AssertType(10, "int");

let eVar22;
AssertType(eVar22, "any");

export let eVar3 = 10, eVar4, eVar5;
AssertType(eVar3, "number");
AssertType(10, "int");
AssertType(eVar4, "any");
AssertType(eVar5, "any");


