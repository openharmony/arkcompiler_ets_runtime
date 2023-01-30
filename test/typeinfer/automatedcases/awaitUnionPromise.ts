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

// === tests/cases/compiler/awaitUnionPromise.ts ===
declare function AssertType(value:any, type:string):void;
// https://github.com/Microsoft/TypeScript/issues/18186

class AsyncEnumeratorDone { };

interface IAsyncEnumerator<T> {
    next1(): Promise<T | AsyncEnumeratorDone>;
    next2(): Promise<T> | Promise<AsyncEnumeratorDone>;
    next3(): Promise<T | {}>;
    next4(): Promise<T | { x: string }>;
}

async function main() {
    const x: IAsyncEnumerator<number> = null;
AssertType(x, "IAsyncEnumerator<number>");
AssertType(null, "null");

    let a = await x.next1();
AssertType(a, "union");
AssertType(await x.next1(), "union");
AssertType(x.next1(), "Promise<union>");
AssertType(x.next1, "() => Promise<union>");

    let b = await x.next2();
AssertType(b, "union");
AssertType(await x.next2(), "union");
AssertType(x.next2(), "union");
AssertType(x.next2, "() => union");

    let c = await x.next3();
AssertType(c, "union");
AssertType(await x.next3(), "union");
AssertType(x.next3(), "Promise<union>");
AssertType(x.next3, "() => Promise<union>");

    let d = await x.next4();
AssertType(d, "union");
AssertType(await x.next4(), "union");
AssertType(x.next4(), "Promise<union>");
AssertType(x.next4, "() => Promise<union>");
}


