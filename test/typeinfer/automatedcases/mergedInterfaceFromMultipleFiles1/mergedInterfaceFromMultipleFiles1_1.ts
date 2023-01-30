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

// === tests/cases/compiler/mergedInterfaceFromMultipleFiles1_1.ts ===
declare function AssertType(value:any, type:string):void;
/// <reference path='mergedInterfaceFromMultipleFiles1_0.ts'/>

interface D { bar(): number; }

interface C extends D {
	b(): Date;
}

let c:C;
AssertType(c, "C");

let a: string = c.foo();
AssertType(a, "string");
AssertType(c.foo(), "string");
AssertType(c.foo, "() => string");

let b: number = c.bar();
AssertType(b, "number");
AssertType(c.bar(), "number");
AssertType(c.bar, "() => number");

let d: number = c.a();
AssertType(d, "number");
AssertType(c.a(), "number");
AssertType(c.a, "() => number");

let e: Date = c.b();
AssertType(e, "Date");
AssertType(c.b(), "Date");
AssertType(c.b, "() => Date");


