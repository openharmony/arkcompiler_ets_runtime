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

// === tests/cases/compiler/moduleCodeGenTest5.ts ===
declare function AssertType(value:any, type:string):void;
export let x = 0;
AssertType(x, "number");
AssertType(0, "int");

let y = 0;
AssertType(y, "number");
AssertType(0, "int");

export function f1() {}
function f2() {}

export class C1 {
	public p1 = 0;
	public p2() {}
}
class C2{
	public p1 = 0;
	public p2() {}	
}

export enum E1 {A=0}
let u = E1.A;
AssertType(u, "E1");
AssertType(E1.A, "E1");

enum E2 {B=0}
let v = E2.B;
AssertType(v, "E2");
AssertType(E2.B, "E2");



