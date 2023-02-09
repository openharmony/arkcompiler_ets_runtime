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

// === tests/cases/compiler/nonNullableReductionNonStrict.ts ===
declare function AssertType(value:any, type:string):void;
// Repros from #43425

type Transform1<T> = ((value: string) => T) | (string extends T ? undefined : never);
type Transform2<T> = string extends T ? ((value: string) => T) | undefined : (value: string) => T;

function test<T>(f1: Transform1<T>, f2: Transform2<T>) {
    f1?.("hello");
AssertType(f1?.("hello"), "T");
AssertType(f1, "(string) => T");
AssertType("hello", "string");

    f2?.("hello");
AssertType(f2?.("hello"), "T");
AssertType(f2, "union");
AssertType("hello", "string");
}

function f1<T>(x: T | (string extends T ? null | undefined : never)) {
    let z = x!;  // NonNullable<T>
AssertType(z, "union");
AssertType(x!, "union");
AssertType(x, "union");
}

function f2<T, U extends null | undefined>(x: T | U) {
    let z = x!;  // NonNullable<T>
AssertType(z, "union");
AssertType(x!, "union");
AssertType(x, "union");
}


