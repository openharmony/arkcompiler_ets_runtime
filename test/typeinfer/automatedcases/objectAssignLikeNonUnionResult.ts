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

// === tests/cases/compiler/objectAssignLikeNonUnionResult.ts ===
declare function AssertType(value:any, type:string):void;
interface Interface {
    field: number;
}
const defaultValue: Interface = { field: 1 };
AssertType(defaultValue, "Interface");
AssertType({ field: 1 }, "{ field: number; }");
AssertType(field, "number");
AssertType(1, "int");

declare function assign<T, U>(target: T, source: U): T & U;

// Displayed type: Interface & { field: number }
// Underlying type: Something else...
const data1 = assign(defaultValue, Date.now() > 3 ? { field: 2 } : {});
AssertType(data1, "Interface & { field: number; }");
AssertType(assign(defaultValue, Date.now() > 3 ? { field: 2 } : {}), "Interface & { field: number; }");
AssertType(assign, "<T, U>(T, U) => T & U");
AssertType(defaultValue, "Interface");
AssertType(Date.now() > 3 ? { field: 2 } : {}, "union");
AssertType(Date.now() > 3, "boolean");
AssertType(Date.now(), "number");
AssertType(Date.now, "() => number");
AssertType(3, "int");
AssertType({ field: 2 }, "{ field: number; }");
AssertType(field, "number");
AssertType(2, "int");
AssertType({}, "{}");

type ExtractRawComponent<T> = T extends { __raw: infer C } ? [L1: T, L2: C] : [R1: T];
type t1 = ExtractRawComponent<typeof data1>;

// ???
type Explode<T> = T extends { x: infer A } ? [A] : 'X';
// 'X' | [unknown] -- why?
type e1 = Explode<typeof data1>;

