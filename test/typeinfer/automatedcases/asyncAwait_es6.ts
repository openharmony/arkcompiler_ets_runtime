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

// === tests/cases/conformance/async/es6/asyncAwait_es6.ts ===
declare function AssertType(value:any, type:string):void;
type MyPromise<T> = Promise<T>;
declare let MyPromise: typeof Promise;
AssertType(MyPromise, "PromiseConstructor");
AssertType(Promise, "PromiseConstructor");

declare let p: Promise<number>;
AssertType(p, "Promise<number>");

declare let mp: MyPromise<number>;
AssertType(mp, "MyPromise<number>");

async function f0() { }
async function f1(): Promise<void> { }
async function f3(): MyPromise<void> { }

let f4 = async function() { 
AssertType(f4, "() => Promise<void>");

AssertType(async function() { }, "() => Promise<void>");
}

let f5 = async function(): Promise<void> { 
AssertType(f5, "() => Promise<void>");

AssertType(async function(): Promise<void> { }, "() => Promise<void>");
}

let f6 = async function(): MyPromise<void> { 
AssertType(f6, "() => MyPromise<void>");

AssertType(async function(): MyPromise<void> { }, "() => MyPromise<void>");
}

let f7 = async () => { };
AssertType(f7, "() => Promise<void>");
AssertType(async () => { }, "() => Promise<void>");

let f8 = async (): Promise<void> => { };
AssertType(f8, "() => Promise<void>");
AssertType(async (): Promise<void> => { }, "() => Promise<void>");

let f9 = async (): MyPromise<void> => { };
AssertType(f9, "() => MyPromise<void>");
AssertType(async (): MyPromise<void> => { }, "() => MyPromise<void>");

let f10 = async () => p;
AssertType(f10, "() => Promise<number>");
AssertType(async () => p, "() => Promise<number>");
AssertType(p, "Promise<number>");

let f11 = async () => mp;
AssertType(f11, "() => Promise<number>");
AssertType(async () => mp, "() => Promise<number>");
AssertType(mp, "MyPromise<number>");

let f12 = async (): Promise<number> => mp;
AssertType(f12, "() => Promise<number>");
AssertType(async (): Promise<number> => mp, "() => Promise<number>");
AssertType(mp, "MyPromise<number>");

let f13 = async (): MyPromise<number> => p;
AssertType(f13, "() => MyPromise<number>");
AssertType(async (): MyPromise<number> => p, "() => MyPromise<number>");
AssertType(p, "Promise<number>");

let o = {
AssertType(o, "{ m1(): Promise<void>; m2(): Promise<void>; m3(): MyPromise<void>; }");
AssertType({	async m1() { },	async m2(): Promise<void> { },	async m3(): MyPromise<void> { }}, "{ m1(): Promise<void>; m2(): Promise<void>; m3(): MyPromise<void>; }");

	async m1() { },
AssertType(m1, "() => Promise<void>");

	async m2(): Promise<void> { },
AssertType(m2, "() => Promise<void>");

	async m3(): MyPromise<void> { 
AssertType(m3, "() => MyPromise<void>");
}

};

class C {
	async m1() { }
	async m2(): Promise<void> { }
	async m3(): MyPromise<void> { }
	static async m4() { }
	static async m5(): Promise<void> { }
	static async m6(): MyPromise<void> { }
}

module M {
	export async function f1() { }
}

async function f14() {
    block: {
AssertType(block, "any");

        await 1;
AssertType(await 1, "int");
AssertType(1, "int");

        break block;
AssertType(block, "any");
    }
}

