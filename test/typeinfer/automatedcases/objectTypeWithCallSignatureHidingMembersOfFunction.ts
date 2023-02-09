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

// === tests/cases/conformance/types/members/objectTypeWithCallSignatureHidingMembersOfFunction.ts ===
declare function AssertType(value:any, type:string):void;
// object types with call signatures can override members of Function
// no errors expected below 

interface I {
    (): void;
    apply(a: any, b?: any): void;
    call(thisArg: number, ...argArray: number[]): any;
}

let i: I;
AssertType(i, "I");

let r1: (a: any, b?: any) => void = i.apply;
AssertType(r1, "(any, ?any) => void");
AssertType(a, "any");
AssertType(b, "any");
AssertType(i.apply, "(any, ?any) => void");

let r1b: (thisArg: number, ...argArray: number[]) => void = i.call;
AssertType(r1b, "(number, ...number[]) => void");
AssertType(thisArg, "number");
AssertType(argArray, "number[]");
AssertType(i.call, "(number, ...number[]) => any");

let r1c = i.arguments;
AssertType(r1c, "any");
AssertType(i.arguments, "any");

let x: {
AssertType(x, "{ (): void; apply(any, ?any): void; call(number, ...number[]): any; }");

    (): void;
    apply(a: any, b?: any): void;
AssertType(apply, "(any, ?any) => void");
AssertType(a, "any");
AssertType(b, "any");

    call(thisArg: number, ...argArray: number[]): any;
AssertType(call, "(number, ...number[]) => any");
AssertType(thisArg, "number");
AssertType(argArray, "number[]");
}

let r2: (a: any, b?: any) => void = x.apply;
AssertType(r2, "(any, ?any) => void");
AssertType(a, "any");
AssertType(b, "any");
AssertType(x.apply, "(any, ?any) => void");

let r2b: (thisArg: number, ...argArray: number[]) => void = x.call;
AssertType(r2b, "(number, ...number[]) => void");
AssertType(thisArg, "number");
AssertType(argArray, "number[]");
AssertType(x.call, "(number, ...number[]) => any");

let r2c = x.arguments;
AssertType(r2c, "any");
AssertType(x.arguments, "any");


