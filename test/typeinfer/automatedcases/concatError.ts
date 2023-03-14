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

// === tests/cases/compiler/concatError.ts ===
declare function AssertType(value:any, type:string):void;
let n1: number[];
AssertType(n1, "number[]");

let fa: number[];
AssertType(fa, "number[]");

fa = fa.concat([0]);
AssertType(fa = fa.concat([0]), "number[]");
AssertType(fa, "number[]");
AssertType(fa.concat([0]), "number[]");
AssertType(fa.concat, "{ (...ConcatArray<number>[]): number[]; (...(union)[]): number[]; }");
AssertType([0], "number[]");
AssertType(0, "int");

fa = fa.concat(0);
AssertType(fa = fa.concat(0), "number[]");
AssertType(fa, "number[]");
AssertType(fa.concat(0), "number[]");
AssertType(fa.concat, "{ (...ConcatArray<number>[]): number[]; (...(union)[]): number[]; }");
AssertType(0, "int");





/*




declare class C<T> {
	public m(p1: C<C<T>>): C<T>;
	//public p: T;
}

let c: C<number>;
let cc: C<C<number>>;

c = c.m(cc);
*/

