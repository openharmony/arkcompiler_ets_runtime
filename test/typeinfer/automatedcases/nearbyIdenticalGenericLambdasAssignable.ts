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

// === tests/cases/compiler/nearbyIdenticalGenericLambdasAssignable.ts ===
declare function AssertType(value:any, type:string):void;
declare const fA: <T>() => { v: T };
AssertType(fA, "<T>() => { v: T; }");
AssertType(v, "T");

const fB = <T>() => {
AssertType(fB, "<T>() => { v: T; }");
AssertType(<T>() => {    return { v: '' as any as T };}, "<T>() => { v: T; }");

AssertType({ v: '' as any as T }, "{ v: T; }");
AssertType(v, "T");
AssertType('' as any as T, "T");
AssertType('' as any, "any");
AssertType('', "string");
    return { v: '' as any as T };

};
const fC = <T>() => {
AssertType(fC, "<T>() => { v: T; }");
AssertType(<T>() => {    return {} as any as { v: T };}, "<T>() => { v: T; }");

AssertType({} as any as { v: T }, "{ v: T; }");
AssertType({} as any, "any");
AssertType({}, "{}");
AssertType(v, "T");
    return {} as any as { v: T };

};

// Hover display is identical on all of these
type TA = typeof fA;
type TB = typeof fB;
type TC = typeof fC;
type TL = <T>() => { v: T }; 

declare function accA(x: TA): void;
declare function accB(x: TB): void;
declare function accC(x: TC): void;
declare function accL(x: TL): void;

// These should all be OK, every type is identical
accA(fA); accA(fB); accA(fC);
AssertType(accA(fA), "void");
AssertType(accA, "(<T>() => { v: T; }) => void");
AssertType(fA, "<T>() => { v: T; }");
AssertType(accA(fB), "void");
AssertType(accA, "(<T>() => { v: T; }) => void");
AssertType(fB, "<T>() => { v: T; }");
AssertType(accA(fC), "void");
AssertType(accA, "(<T>() => { v: T; }) => void");
AssertType(fC, "<T>() => { v: T; }");

//             ~~ previously an error
accB(fA); accB(fB); accB(fC);
AssertType(accB(fA), "void");
AssertType(accB, "(<T>() => { v: T; }) => void");
AssertType(fA, "<T>() => { v: T; }");
AssertType(accB(fB), "void");
AssertType(accB, "(<T>() => { v: T; }) => void");
AssertType(fB, "<T>() => { v: T; }");
AssertType(accB(fC), "void");
AssertType(accB, "(<T>() => { v: T; }) => void");
AssertType(fC, "<T>() => { v: T; }");

//             OK
accC(fA); accC(fB); accC(fC);
AssertType(accC(fA), "void");
AssertType(accC, "(<T>() => { v: T; }) => void");
AssertType(fA, "<T>() => { v: T; }");
AssertType(accC(fB), "void");
AssertType(accC, "(<T>() => { v: T; }) => void");
AssertType(fB, "<T>() => { v: T; }");
AssertType(accC(fC), "void");
AssertType(accC, "(<T>() => { v: T; }) => void");
AssertType(fC, "<T>() => { v: T; }");

//             ~~ previously an error
accL(fA); accL(fB); accL(fC);
AssertType(accL(fA), "void");
AssertType(accL, "(TL) => void");
AssertType(fA, "<T>() => { v: T; }");
AssertType(accL(fB), "void");
AssertType(accL, "(TL) => void");
AssertType(fB, "<T>() => { v: T; }");
AssertType(accL(fC), "void");
AssertType(accL, "(TL) => void");
AssertType(fC, "<T>() => { v: T; }");

//             ~~ previously an error

