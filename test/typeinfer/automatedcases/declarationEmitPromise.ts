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

// === tests/cases/compiler/declarationEmitPromise.ts ===
declare function AssertType(value:any, type:string):void;
export class bluebird<T> {
    static all: Array<bluebird<any>>;
}

export async function runSampleWorks<A, B, C, D, E>(
    a: bluebird<A>, b?: bluebird<B>, c?: bluebird<C>, d?: bluebird<D>, e?: bluebird<E>) {
    let result = await (bluebird.all as any)([a, b, c, d, e].filter(el => !!el));
AssertType(result, "any");
AssertType(await (bluebird.all as any)([a, b, c, d, e].filter(el => !!el)), "any");
AssertType((bluebird.all as any)([a, b, c, d, e].filter(el => !!el)), "any");
AssertType((bluebird.all as any), "any");
AssertType(bluebird.all as any, "any");
AssertType(bluebird.all, "bluebird<any>[]");
AssertType([a, b, c, d, e].filter(el => !!el), "bluebird<A>[]");
AssertType([a, b, c, d, e].filter, "{ <S extends bluebird<A>>((bluebird<A>, number, bluebird<A>[]) => value is S, ?any): S[]; ((bluebird<A>, number, bluebird<A>[]) => unknown, ?any): bluebird<A>[]; }");
AssertType([a, b, c, d, e], "bluebird<A>[]");
AssertType(a, "bluebird<A>");
AssertType(b, "bluebird<B>");
AssertType(c, "bluebird<C>");
AssertType(d, "bluebird<D>");
AssertType(e, "bluebird<E>");
AssertType(el => !!el, "(bluebird<A>) => boolean");
AssertType(el, "bluebird<A>");
AssertType(!!el, "boolean");
AssertType(!el, "boolean");
AssertType(el, "bluebird<A>");

    let func = <T>(f: (a: A, b?: B, c?: C, d?: D, e?: E) => T): T =>
AssertType(func, "<T>((A, ?B, ?C, ?D, ?E) => T) => T");
AssertType(<T>(f: (a: A, b?: B, c?: C, d?: D, e?: E) => T): T =>        f.apply(this, result), "<T>((A, ?B, ?C, ?D, ?E) => T) => T");
AssertType(f, "(A, ?B, ?C, ?D, ?E) => T");
AssertType(a, "A");
AssertType(b, "B");
AssertType(c, "C");
AssertType(d, "D");
AssertType(e, "E");

        f.apply(this, result);
AssertType(f.apply(this, result), "any");
AssertType(f.apply, "(Function, any, ?any) => any");
AssertType(this, "any");
AssertType(result, "any");

    let rfunc: typeof func & {} = func as any; // <- This is the only difference
AssertType(rfunc, "<T>((A, ?B, ?C, ?D, ?E) => T) => T");
AssertType(func, "<T>((A, ?B, ?C, ?D, ?E) => T) => T");
AssertType(func as any, "any");
AssertType(func, "<T>((A, ?B, ?C, ?D, ?E) => T) => T");

AssertType(rfunc, "<T>((A, ?B, ?C, ?D, ?E) => T) => T");
    return rfunc
}

export async function runSampleBreaks<A, B, C, D, E>(
    a: bluebird<A>, b?: bluebird<B>, c?: bluebird<C>, d?: bluebird<D>, e?: bluebird<E>) {
    let result = await (bluebird.all as any)([a, b, c, d, e].filter(el => !!el));
AssertType(result, "any");
AssertType(await (bluebird.all as any)([a, b, c, d, e].filter(el => !!el)), "any");
AssertType((bluebird.all as any)([a, b, c, d, e].filter(el => !!el)), "any");
AssertType((bluebird.all as any), "any");
AssertType(bluebird.all as any, "any");
AssertType(bluebird.all, "bluebird<any>[]");
AssertType([a, b, c, d, e].filter(el => !!el), "bluebird<A>[]");
AssertType([a, b, c, d, e].filter, "{ <S extends bluebird<A>>((bluebird<A>, number, bluebird<A>[]) => value is S, ?any): S[]; ((bluebird<A>, number, bluebird<A>[]) => unknown, ?any): bluebird<A>[]; }");
AssertType([a, b, c, d, e], "bluebird<A>[]");
AssertType(a, "bluebird<A>");
AssertType(b, "bluebird<B>");
AssertType(c, "bluebird<C>");
AssertType(d, "bluebird<D>");
AssertType(e, "bluebird<E>");
AssertType(el => !!el, "(bluebird<A>) => boolean");
AssertType(el, "bluebird<A>");
AssertType(!!el, "boolean");
AssertType(!el, "boolean");
AssertType(el, "bluebird<A>");

    let func = <T>(f: (a: A, b?: B, c?: C, d?: D, e?: E) => T): T =>
AssertType(func, "<T>((A, ?B, ?C, ?D, ?E) => T) => T");
AssertType(<T>(f: (a: A, b?: B, c?: C, d?: D, e?: E) => T): T =>        f.apply(this, result), "<T>((A, ?B, ?C, ?D, ?E) => T) => T");
AssertType(f, "(A, ?B, ?C, ?D, ?E) => T");
AssertType(a, "A");
AssertType(b, "B");
AssertType(c, "C");
AssertType(d, "D");
AssertType(e, "E");

        f.apply(this, result);
AssertType(f.apply(this, result), "any");
AssertType(f.apply, "(Function, any, ?any) => any");
AssertType(this, "any");
AssertType(result, "any");

    let rfunc: typeof func = func as any; // <- This is the only difference
AssertType(rfunc, "<T>((A, ?B, ?C, ?D, ?E) => T) => T");
AssertType(func, "<T>((A, ?B, ?C, ?D, ?E) => T) => T");
AssertType(func as any, "any");
AssertType(func, "<T>((A, ?B, ?C, ?D, ?E) => T) => T");

AssertType(rfunc, "<T>((A, ?B, ?C, ?D, ?E) => T) => T");
    return rfunc
}

