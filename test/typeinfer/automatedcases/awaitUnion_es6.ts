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

// === tests/cases/conformance/async/es6/awaitUnion_es6.ts ===
declare function AssertType(value:any, type:string):void;
declare let a: number | string;
AssertType(a, "union");

declare let b: PromiseLike<number> | PromiseLike<string>;
AssertType(b, "union");

declare let c: PromiseLike<number | string>;
AssertType(c, "PromiseLike<union>");

declare let d: number | PromiseLike<string>;
AssertType(d, "union");

declare let e: number | PromiseLike<number | string>;
AssertType(e, "union");

async function f() {
	let await_a = await a;
AssertType(await_a, "union");
AssertType(await a, "union");
AssertType(a, "union");

	let await_b = await b;
AssertType(await_b, "union");
AssertType(await b, "union");
AssertType(b, "union");

	let await_c = await c;
AssertType(await_c, "union");
AssertType(await c, "union");
AssertType(c, "PromiseLike<union>");

	let await_d = await d;
AssertType(await_d, "union");
AssertType(await d, "union");
AssertType(d, "union");

	let await_e = await e;
AssertType(await_e, "union");
AssertType(await e, "union");
AssertType(e, "union");
}

