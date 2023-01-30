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

// === tests/cases/compiler/typeGuardConstructorNarrowPrimitivesInUnion.ts ===
declare function AssertType(value:any, type:string):void;
// Union of primitives, number, arrays, and C1
let var1: number | "hello" | "world" | true | false | number[] | string[];
AssertType(var1, "union");
AssertType(true, "boolean");
AssertType(false, "boolean");

if (var1.constructor === Number) {
    var1; // number
AssertType(var1, "number");
}

if (var1.constructor === String) {
    var1; // "hello" | "world"
AssertType(var1, "union");
}

if (var1.constructor === Boolean) {
    var1; // boolean
AssertType(var1, "boolean");
}

if (var1.constructor === Array) {
    var1; // number[] | string[]
AssertType(var1, "union");
}


