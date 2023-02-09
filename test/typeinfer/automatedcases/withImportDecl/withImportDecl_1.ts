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

// === tests/cases/compiler/withImportDecl_1.ts ===
declare function AssertType(value:any, type:string):void;
///<reference path='withImportDecl_0.ts'/>
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

declare let declaredVar;
AssertType(declaredVar, "any");

declare let deckareVarWithType: number;
AssertType(deckareVarWithType, "number");

let arrayVar: string[] = ['a', 'b'];
AssertType(arrayVar, "string[]");
AssertType(['a', 'b'], "string[]");
AssertType('a', "string");
AssertType('b', "string");


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

module m1 {
    export function foo() {
AssertType("Hello", "string");
        return "Hello";
    }
}

import * as m3 from "withImportDecl_0";

let b = new m3.A();
AssertType(b, "m3.A");
AssertType(new m3.A(), "m3.A");
AssertType(m3.A, "typeof m3.A");

b.foo;
AssertType(b.foo, "string");


