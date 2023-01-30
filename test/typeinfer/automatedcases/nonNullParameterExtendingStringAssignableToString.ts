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

// === tests/cases/compiler/nonNullParameterExtendingStringAssignableToString.ts ===
declare function AssertType(value:any, type:string):void;
declare function foo(p: string): void;

function fn<T extends string | undefined, U extends string>(one: T, two: U) {
    let three = Boolean() ? one : two;
AssertType(three, "union");
AssertType(Boolean() ? one : two, "union");
AssertType(Boolean(), "boolean");
AssertType(Boolean, "BooleanConstructor");
AssertType(one, "T");
AssertType(two, "U");

    foo(one!);
AssertType(foo(one!), "void");
AssertType(foo, "(string) => void");
AssertType(one!, "string");
AssertType(one, "union");

    foo(two!);
AssertType(foo(two!), "void");
AssertType(foo, "(string) => void");
AssertType(two!, "U");
AssertType(two, "U");

    foo(three!); // this line is the important one
AssertType(foo(three!), "void");
AssertType(foo, "(string) => void");
AssertType(three!, "string");
AssertType(three, "union");
}

