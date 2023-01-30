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

// === tests/cases/conformance/types/typeRelationships/typeInference/keyofInferenceIntersectsResults.ts ===
declare function AssertType(value:any, type:string):void;
interface X {
    a: string;
    b: string;
}

declare function foo<T = X>(x: keyof T, y: keyof T): T;
declare function bar<T>(x: keyof T, y: keyof T): T;

const a = foo<X>('a', 'b'); // compiles cleanly
AssertType(a, "X");
AssertType(foo<X>('a', 'b'), "X");
AssertType(foo, "<T = X>(keyof T, keyof T) => T");
AssertType('a', "string");
AssertType('b', "string");

const b = foo('a', 'b');    // also clean
AssertType(b, "{ a: any; } & { b: any; }");
AssertType(foo('a', 'b'), "{ a: any; } & { b: any; }");
AssertType(foo, "<T = X>(keyof T, keyof T) => T");
AssertType('a', "string");
AssertType('b', "string");

const c = bar('a', 'b');    // still clean
AssertType(c, "{ a: any; } & { b: any; }");
AssertType(bar('a', 'b'), "{ a: any; } & { b: any; }");
AssertType(bar, "<T>(keyof T, keyof T) => T");
AssertType('a', "string");
AssertType('b', "string");


