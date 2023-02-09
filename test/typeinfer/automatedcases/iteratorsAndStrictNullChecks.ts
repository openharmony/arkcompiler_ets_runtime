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

// === tests/cases/compiler/iteratorsAndStrictNullChecks.ts ===
declare function AssertType(value:any, type:string):void;
// for..of
for (const x of ["a", "b"]) {
    x.substring;
AssertType(x.substring, "(number, ?union) => string");
}

// Spread
const xs = [1, 2, 3];
AssertType(xs, "number[]");
AssertType([1, 2, 3], "number[]");
AssertType(1, "int");
AssertType(2, "int");
AssertType(3, "int");

const ys = [4, 5];
AssertType(ys, "number[]");
AssertType([4, 5], "number[]");
AssertType(4, "int");
AssertType(5, "int");

xs.push(...ys);
AssertType(xs.push(...ys), "number");
AssertType(xs.push, "(...number[]) => number");
AssertType(...ys, "number");
AssertType(ys, "number[]");


