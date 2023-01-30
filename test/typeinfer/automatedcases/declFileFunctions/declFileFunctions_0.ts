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

// === tests/cases/compiler/declFileFunctions_0.ts ===
declare function AssertType(value:any, type:string):void;
/** This comment should appear for foo*/
export function foo() {
}
/** This is comment for function signature*/
export function fooWithParameters(/** this is comment about a*/a: string,
    /** this is comment for b*/
    b: number) {
    let d = a;
AssertType(d, "string");
AssertType(a, "string");
}
export function fooWithRestParameters(a: string, ...rests: string[]) {
AssertType(a + rests.join(""), "string");
AssertType(a, "string");
AssertType(rests.join(""), "string");
AssertType(rests.join, "(?string) => string");
AssertType("", "string");
    return a + rests.join("");
}

export function fooWithOverloads(a: string): string;
export function fooWithOverloads(a: number): number;
export function fooWithOverloads(a: any): any {
AssertType(a, "any");
    return a;
}

export function fooWithSingleOverload(a: string): string;
export function fooWithSingleOverload(a: any) {
AssertType(a, "any");
    return a;
}

export function fooWithTypePredicate(a: any): a is number {
AssertType(true, "boolean");
    return true;
}
export function fooWithTypePredicateAndMulitpleParams(a: any, b: any, c: any): a is number {
AssertType(true, "boolean");
    return true;
}
export function fooWithTypeTypePredicateAndGeneric<T>(a: any): a is T {
AssertType(true, "boolean");
    return true;
}
export function fooWithTypeTypePredicateAndRestParam(a: any, ...rest): a is number {
AssertType(true, "boolean");
    return true;
}

/** This comment should appear for nonExportedFoo*/
function nonExportedFoo() {
}
/** This is comment for function signature*/
function nonExportedFooWithParameters(/** this is comment about a*/a: string,
    /** this is comment for b*/
    b: number) {
    let d = a;
AssertType(d, "string");
AssertType(a, "string");
}
function nonExportedFooWithRestParameters(a: string, ...rests: string[]) {
AssertType(a + rests.join(""), "string");
AssertType(a, "string");
AssertType(rests.join(""), "string");
AssertType(rests.join, "(?string) => string");
AssertType("", "string");
    return a + rests.join("");
}

function nonExportedFooWithOverloads(a: string): string;
function nonExportedFooWithOverloads(a: number): number;
function nonExportedFooWithOverloads(a: any): any {
AssertType(a, "any");
    return a;
}


