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

// === tests/cases/conformance/classes/members/privateNames/privateNameMethodAsync.ts ===
declare function AssertType(value:any, type:string):void;
const C = class {
AssertType(C, "typeof C");
AssertType(class {    async #bar() { return await Promise.resolve(42); }    async foo() {        const b = await this.#bar();        return b + (this.#baz().next().value || 0) + ((await this.#qux().next()).value || 0);    }    *#baz() { yield 42; }    async *#qux() {        yield (await Promise.resolve(42));    }}, "typeof C");

    async #bar() { 
AssertType(#bar, "() => Promise<number>");
return await Promise.resolve(42); 

AssertType(await Promise.resolve(42), "number");

AssertType(Promise.resolve(42), "Promise<number>");

AssertType(Promise.resolve, "{ (): Promise<void>; <T>(T): Promise<Awaited<T>>; <T>(union): Promise<Awaited<T>>; }");

AssertType(42, "int");
}

    async foo() {
AssertType(foo, "() => Promise<number>");

        const b = await this.#bar();
AssertType(b, "number");
AssertType(await this.#bar(), "number");
AssertType(this.#bar(), "Promise<number>");
AssertType(this.#bar, "() => Promise<number>");
AssertType(this, "this");

AssertType(b + (this.#baz().next().value || 0) + ((await this.#qux().next()).value || 0), "number");
AssertType(b + (this.#baz().next().value || 0), "number");
AssertType(b, "number");
AssertType((this.#baz().next().value || 0), "number");
AssertType(this.#baz().next().value || 0, "number");
AssertType(this.#baz().next().value, "union");
AssertType(this.#baz().next(), "IteratorResult<number, void>");
AssertType(this.#baz().next, "(...union) => IteratorResult<number, void>");
AssertType(this.#baz(), "Generator<number, void, unknown>");
AssertType(this.#baz, "() => Generator<number, void, unknown>");
AssertType(this, "this");
AssertType(0, "int");
AssertType(((await this.#qux().next()).value || 0), "number");
AssertType((await this.#qux().next()).value || 0, "number");
AssertType((await this.#qux().next()).value, "union");
AssertType((await this.#qux().next()), "IteratorResult<number, void>");
AssertType(await this.#qux().next(), "IteratorResult<number, void>");
AssertType(this.#qux().next(), "Promise<IteratorResult<number, void>>");
AssertType(this.#qux().next, "(...union) => Promise<IteratorResult<number, void>>");
AssertType(this.#qux(), "AsyncGenerator<number, void, unknown>");
AssertType(this.#qux, "() => AsyncGenerator<number, void, unknown>");
AssertType(this, "this");
AssertType(0, "int");
        return b + (this.#baz().next().value || 0) + ((await this.#qux().next()).value || 0);
    }
    *#baz() { yield 42; 
AssertType(#baz, "() => Generator<number, void, unknown>");

AssertType(yield 42, "any");

AssertType(42, "int");
}

    async *#qux() {
AssertType(#qux, "() => AsyncGenerator<number, void, unknown>");

        yield (await Promise.resolve(42));
AssertType(yield (await Promise.resolve(42)), "any");
AssertType((await Promise.resolve(42)), "number");
AssertType(await Promise.resolve(42), "number");
AssertType(Promise.resolve(42), "Promise<number>");
AssertType(Promise.resolve, "{ (): Promise<void>; <T>(T): Promise<Awaited<T>>; <T>(union): Promise<Awaited<T>>; }");
AssertType(42, "int");
    }
}

new C().foo().then(console.log);
AssertType(new C().foo().then(console.log), "Promise<void>");
AssertType(new C().foo().then, "<TResult1 = number, TResult2 = never>(?(number) => union, ?(any) => union) => Promise<union>");
AssertType(console.log, "(...any[]) => void");


