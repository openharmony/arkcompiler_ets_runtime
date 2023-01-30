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

// === tests/cases/compiler/multiExtendsSplitInterfaces2.ts ===
declare function AssertType(value:any, type:string):void;
interface A {
	a: number;
}

interface I extends A {
	i1: number;
}

interface B {
	b: number;
}

interface I extends B {
	i2: number;
}

let i: I;
AssertType(i, "I");

let a = i.a;
AssertType(a, "number");
AssertType(i.a, "number");

let i1 = i.i1;
AssertType(i1, "number");
AssertType(i.i1, "number");

let b = i.b;
AssertType(b, "number");
AssertType(i.b, "number");

let i2 = i.i2;
AssertType(i2, "number");
AssertType(i.i2, "number");


