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

// === tests/cases/compiler/inheritedOverloadedSpecializedSignatures.ts ===
declare function AssertType(value:any, type:string):void;
interface A {
  (key:string):void;
}

interface B extends A {
  (key:'foo'):string;
}

let b:B;
AssertType(b, "B");

// Should not error
b('foo').charAt(0);
AssertType(b('foo').charAt(0), "string");
AssertType(b('foo').charAt, "(number) => string");
AssertType(0, "int");

interface A {
    (x: 'A1'): string;
    (x: string): void;
}

interface B extends A {
    (x: 'B1'): number;
}

interface A {
    (x: 'A2'): boolean;
}

interface B  {
    (x: 'B2'): string[];
}

interface C1 extends B {
	(x: 'C1'): number[];
}

interface C2 extends B {
	(x: 'C2'): boolean[];
}

interface C extends C1, C2 {
	(x: 'C'): string;
}

let c: C;
AssertType(c, "C");

// none of these lines should error
let x1: string[] = c('B2');
AssertType(x1, "string[]");
AssertType(c('B2'), "string[]");
AssertType(c, "C");
AssertType('B2', "string");

let x2: number = c('B1');
AssertType(x2, "number");
AssertType(c('B1'), "number");
AssertType(c, "C");
AssertType('B1', "string");

let x3: boolean = c('A2');
AssertType(x3, "boolean");
AssertType(c('A2'), "boolean");
AssertType(c, "C");
AssertType('A2', "string");

let x4: string = c('A1');
AssertType(x4, "string");
AssertType(c('A1'), "string");
AssertType(c, "C");
AssertType('A1', "string");

let x5: void = c('A0');
AssertType(x5, "void");
AssertType(c('A0'), "void");
AssertType(c, "C");
AssertType('A0', "string");

let x6: number[] = c('C1');
AssertType(x6, "number[]");
AssertType(c('C1'), "number[]");
AssertType(c, "C");
AssertType('C1', "string");

let x7: boolean[] = c('C2');
AssertType(x7, "boolean[]");
AssertType(c('C2'), "boolean[]");
AssertType(c, "C");
AssertType('C2', "string");

let x8: string = c('C');
AssertType(x8, "string");
AssertType(c('C'), "string");
AssertType(c, "C");
AssertType('C', "string");

let x9: void = c('generic');
AssertType(x9, "void");
AssertType(c('generic'), "void");
AssertType(c, "C");
AssertType('generic', "string");


