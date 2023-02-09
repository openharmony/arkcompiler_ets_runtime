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

// === tests/cases/compiler/typeInferenceCacheInvalidation.ts ===
declare function AssertType(value:any, type:string):void;
// Repro from #32230

type Callback<TFoo, TBar> = (foo: TFoo, bar: TBar) => any

declare function example<TFoo, TBar, TCallback extends Callback<TFoo, TBar>>(
    foo: TFoo,
    callback: TCallback,
    bar: TBar,
): TCallback

example(42, (foo, bar) => ({
AssertType(example(42, (foo, bar) => ({    t: () => {        let s: string = bar;    }}), '42'), "(number, string) => { t: () => void; }");
AssertType(example, "<TFoo, TBar, TCallback extends Callback<TFoo, TBar>>(TFoo, TCallback, TBar) => TCallback");
AssertType(42, "int");
AssertType((foo, bar) => ({    t: () => {        let s: string = bar;    }}), "(number, string) => { t: () => void; }");
AssertType(foo, "number");
AssertType(bar, "string");
AssertType(({    t: () => {        let s: string = bar;    }}), "{ t: () => void; }");
AssertType({    t: () => {        let s: string = bar;    }}, "{ t: () => void; }");

    t: () => {
AssertType(t, "() => void");
AssertType(() => {        let s: string = bar;    }, "() => void");

        let s: string = bar;
AssertType(s, "string");
AssertType(bar, "string");
    }
}), '42');
AssertType('42', "string");

example(42, (foo, bar) => ({
AssertType(example(42, (foo, bar) => ({    t() {        let s: string = bar;    }}), '42'), "(number, string) => { t(): void; }");
AssertType(example, "<TFoo, TBar, TCallback extends Callback<TFoo, TBar>>(TFoo, TCallback, TBar) => TCallback");
AssertType(42, "int");
AssertType((foo, bar) => ({    t() {        let s: string = bar;    }}), "(number, string) => { t(): void; }");
AssertType(foo, "number");
AssertType(bar, "string");
AssertType(({    t() {        let s: string = bar;    }}), "{ t(): void; }");
AssertType({    t() {        let s: string = bar;    }}, "{ t(): void; }");

    t() {
AssertType(t, "() => void");

        let s: string = bar;
AssertType(s, "string");
AssertType(bar, "string");
    }
}), '42');
AssertType('42', "string");


