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

// === tests/cases/compiler/genericFunctionsAndConditionalInference.ts ===
declare function AssertType(value:any, type:string):void;
type Boxified<T> = { [P in keyof T]: { value: T[P]} };

declare function unboxify<T>(obj: Boxified<T>): T;

function foo<U, V>(obj: { u: { value: U }, v: { value: V } }) {
AssertType(unboxify(obj), "{ u: U; v: V; }");
AssertType(unboxify, "<T>(Boxified<T>) => T");
AssertType(obj, "{ u: { value: U; }; v: { value: V; }; }");
    return unboxify(obj);
}

let qq = foo({ u: { value: 10 }, v: { value: 'hello'} });  // { u: U, v: V } but should be { u: number, v: string 
AssertType(qq, "{ u: number; v: string; }");

AssertType(foo({ u: { value: 10 }, v: { value: 'hello'} }), "{ u: number; v: string; }");

AssertType(foo, "<U, V>({ u: { value: U; }; v: { value: V; }; }) => { u: U; v: V; }");

AssertType({ u: { value: 10 }, v: { value: 'hello'} }, "{ u: { value: number; }; v: { value: string; }; }");

AssertType(u, "{ value: number; }");

AssertType({ value: 10 }, "{ value: number; }");

AssertType(value, "number");

AssertType(10, "int");

AssertType(v, "{ value: string; }");

AssertType({ value: 'hello'}, "{ value: string; }");

AssertType(value, "string");

AssertType('hello', "string");
}

// From #42385
interface Targets<A> {
    left: A
    right: A
}
type Target = keyof Targets<any>
type Result<F extends Target, A> = Targets<A>[F]

type LR<F extends Target, L, R> = [F] extends ["left"] ? L : R

interface Ops<F extends Target> {
    _f: F
    str: Result<F, string>
    num: Result<F, number>
    lr<I, O>(a: Result<F, I>, o: Result<F, O>): Result<F, LR<F, I, O>>
    dict: <P>(p: {[k in keyof P]: Result<F, P[k]>}) => Result<F, P>
}
const left: Ops<"left"> = {} as any
AssertType(left, "Ops<"left">");
AssertType({} as any, "any");
AssertType({}, "{}");

const right: Ops<"right"> = {} as any
AssertType(right, "Ops<"right">");
AssertType({} as any, "any");
AssertType({}, "{}");

const ok = <F extends Target>(at: Ops<F>) => ({lr: at.lr(at.str, at.num)})
AssertType(ok, "<F extends keyof Targets<any>>(Ops<F>) => { lr: Result<F, LR<F, string, number>>; }");
AssertType(<F extends Target>(at: Ops<F>) => ({lr: at.lr(at.str, at.num)}), "<F extends keyof Targets<any>>(Ops<F>) => { lr: Result<F, LR<F, string, number>>; }");
AssertType(at, "Ops<F>");
AssertType(({lr: at.lr(at.str, at.num)}), "{ lr: Result<F, LR<F, string, number>>; }");
AssertType({lr: at.lr(at.str, at.num)}, "{ lr: Result<F, LR<F, string, number>>; }");
AssertType(lr, "Result<F, LR<F, string, number>>");
AssertType(at.lr(at.str, at.num), "Result<F, LR<F, string, number>>");
AssertType(at.lr, "<I, O>(Result<F, I>, Result<F, O>) => Result<F, LR<F, I, O>>");
AssertType(at.str, "Result<F, string>");
AssertType(at.num, "Result<F, number>");

const orphaned = <F extends Target>(at: Ops<F>) => at.dict(ok(at))
AssertType(orphaned, "<F extends keyof Targets<any>>(Ops<F>) => Result<F, { lr: LR<F, string, number>; }>");
AssertType(<F extends Target>(at: Ops<F>) => at.dict(ok(at)), "<F extends keyof Targets<any>>(Ops<F>) => Result<F, { lr: LR<F, string, number>; }>");
AssertType(at, "Ops<F>");
AssertType(at.dict(ok(at)), "Result<F, { lr: LR<F, string, number>; }>");
AssertType(at.dict, "<P>({ [k in keyof P]: Result<F, P[k]>; }) => Result<F, P>");
AssertType(ok(at), "{ lr: Result<F, LR<F, string, number>>; }");
AssertType(ok, "<F extends keyof Targets<any>>(Ops<F>) => { lr: Result<F, LR<F, string, number>>; }");
AssertType(at, "Ops<F>");

const leftOk = ok(left)
AssertType(leftOk, "{ lr: string; }");
AssertType(ok(left), "{ lr: string; }");
AssertType(ok, "<F extends keyof Targets<any>>(Ops<F>) => { lr: Result<F, LR<F, string, number>>; }");
AssertType(left, "Ops<"left">");

const leftOrphaned = orphaned(left)
AssertType(leftOrphaned, "{ lr: string; }");
AssertType(orphaned(left), "{ lr: string; }");
AssertType(orphaned, "<F extends keyof Targets<any>>(Ops<F>) => Result<F, { lr: LR<F, string, number>; }>");
AssertType(left, "Ops<"left">");

const rightOk = ok(right)
AssertType(rightOk, "{ lr: number; }");
AssertType(ok(right), "{ lr: number; }");
AssertType(ok, "<F extends keyof Targets<any>>(Ops<F>) => { lr: Result<F, LR<F, string, number>>; }");
AssertType(right, "Ops<"right">");

const rightOrphaned = orphaned(right)
AssertType(rightOrphaned, "{ lr: number; }");
AssertType(orphaned(right), "{ lr: number; }");
AssertType(orphaned, "<F extends keyof Targets<any>>(Ops<F>) => Result<F, { lr: LR<F, string, number>; }>");
AssertType(right, "Ops<"right">");


