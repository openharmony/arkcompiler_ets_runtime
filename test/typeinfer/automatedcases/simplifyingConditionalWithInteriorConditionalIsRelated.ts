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

// === tests/cases/compiler/simplifyingConditionalWithInteriorConditionalIsRelated.ts ===
declare function AssertType(value:any, type:string):void;
// from https://github.com/microsoft/TypeScript/issues/30706
type ConditionalType<T> = T extends string ? string : number;

function ConditionalOrUndefined<T>(): ConditionalType<T> | undefined {
AssertType(0 as any, "any");
AssertType(0, "int");
    return 0 as any;
}

function JustConditional<T>(): ConditionalType<T> {
AssertType(ConditionalOrUndefined<T>()!, "ConditionalType<T>");
AssertType(ConditionalOrUndefined<T>(), "union");
AssertType(ConditionalOrUndefined, "<T>() => union");
    return ConditionalOrUndefined<T>()!; // shouldn't error
}


// For comparison...
function genericOrUndefined<T>(): T | undefined {
AssertType(0 as any, "any");
AssertType(0, "int");
    return 0 as any;
}

function JustGeneric<T>(): T {
AssertType(genericOrUndefined<T>()!, "NonNullable<T>");
AssertType(genericOrUndefined<T>(), "union");
AssertType(genericOrUndefined, "<T>() => union");
    return genericOrUndefined<T>()!; // no error
}

// Simplified example:

function f<T>() {
    type One = T extends string ? string : string;
AssertType(One, "T extends string ? string : string");

    type A = T extends number ? One : never;
AssertType(A, "T extends number ? T extends string ? string : string : never");

    const x: One = null as any as A;
AssertType(x, "T extends string ? string : string");
AssertType(null as any as A, "T extends number ? T extends string ? string : string : never");
AssertType(null as any, "any");
AssertType(null, "null");
}


