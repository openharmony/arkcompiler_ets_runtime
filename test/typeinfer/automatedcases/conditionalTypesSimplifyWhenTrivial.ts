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

// === tests/cases/compiler/conditionalTypesSimplifyWhenTrivial.ts ===
declare function AssertType(value:any, type:string):void;
const fn1 = <Params>(
AssertType(fn1, "<Params>(Pick<Params, Exclude<keyof Params, never>>) => Params");
AssertType(<Params>(    params: Pick<Params, Exclude<keyof Params, never>>,): Params => params, "<Params>(Pick<Params, Exclude<keyof Params, never>>) => Params");

    params: Pick<Params, Exclude<keyof Params, never>>,
AssertType(params, "Pick<Params, Exclude<keyof Params, never>>");

): Params => params;
AssertType(params, "Pick<Params, Exclude<keyof Params, never>>");

function fn2<T>(x: Exclude<T, never>) {
    let y: T = x;
AssertType(y, "T");
AssertType(x, "Exclude<T, never>");

    x = y;
AssertType(x = y, "T");
AssertType(x, "Exclude<T, never>");
AssertType(y, "T");
}

const fn3 = <Params>(
AssertType(fn3, "<Params>(Pick<Params, Extract<keyof Params, keyof Params>>) => Params");
AssertType(<Params>(    params: Pick<Params, Extract<keyof Params, keyof Params>>,): Params => params, "<Params>(Pick<Params, Extract<keyof Params, keyof Params>>) => Params");

    params: Pick<Params, Extract<keyof Params, keyof Params>>,
AssertType(params, "Pick<Params, Extract<keyof Params, keyof Params>>");

): Params => params;
AssertType(params, "Pick<Params, Extract<keyof Params, keyof Params>>");

function fn4<T>(x: Extract<T, T>) {
    let y: T = x;
AssertType(y, "T");
AssertType(x, "Extract<T, T>");

    x = y;
AssertType(x = y, "T");
AssertType(x, "Extract<T, T>");
AssertType(y, "T");
}

declare let x: Extract<number | string, any>; // Should be `numebr | string` and not `any`
AssertType(x, "union");

type ExtractWithDefault<T, U, D = never> = T extends U ? T : D;

type ExcludeWithDefault<T, U, D = never> = T extends U ? D : T;

const fn5 = <Params>(
AssertType(fn5, "<Params>(Pick<Params, ExcludeWithDefault<keyof Params, never, never>>) => Params");
AssertType(<Params>(    params: Pick<Params, ExcludeWithDefault<keyof Params, never>>,): Params => params, "<Params>(Pick<Params, ExcludeWithDefault<keyof Params, never, never>>) => Params");

    params: Pick<Params, ExcludeWithDefault<keyof Params, never>>,
AssertType(params, "Pick<Params, ExcludeWithDefault<keyof Params, never, never>>");

): Params => params;
AssertType(params, "Pick<Params, ExcludeWithDefault<keyof Params, never, never>>");

function fn6<T>(x: ExcludeWithDefault<T, never>) {
    let y: T = x;
AssertType(y, "T");
AssertType(x, "ExcludeWithDefault<T, never, never>");

    x = y;
AssertType(x = y, "T");
AssertType(x, "ExcludeWithDefault<T, never, never>");
AssertType(y, "T");
}

const fn7 = <Params>(
AssertType(fn7, "<Params>(Pick<Params, ExtractWithDefault<keyof Params, keyof Params, never>>) => Params");
AssertType(<Params>(    params: Pick<Params, ExtractWithDefault<keyof Params, keyof Params>>,): Params => params, "<Params>(Pick<Params, ExtractWithDefault<keyof Params, keyof Params, never>>) => Params");

    params: Pick<Params, ExtractWithDefault<keyof Params, keyof Params>>,
AssertType(params, "Pick<Params, ExtractWithDefault<keyof Params, keyof Params, never>>");

): Params => params;
AssertType(params, "Pick<Params, ExtractWithDefault<keyof Params, keyof Params, never>>");

function fn8<T>(x: ExtractWithDefault<T, T>) {
    let y: T = x;
AssertType(y, "T");
AssertType(x, "ExtractWithDefault<T, T, never>");

    x = y;
AssertType(x = y, "T");
AssertType(x, "ExtractWithDefault<T, T, never>");
AssertType(y, "T");
}

type TemplatedConditional<TCheck, TExtends, TTrue, TFalse> = TCheck extends TExtends ? TTrue : TFalse;

const fn9 = <Params>(
AssertType(fn9, "<Params>(Pick<Params, TemplatedConditional<keyof Params, never, never, keyof Params>>) => Params");
AssertType(<Params>(    params: Pick<Params, TemplatedConditional<keyof Params, never, never, keyof Params>>,): Params => params, "<Params>(Pick<Params, TemplatedConditional<keyof Params, never, never, keyof Params>>) => Params");

    params: Pick<Params, TemplatedConditional<keyof Params, never, never, keyof Params>>,
AssertType(params, "Pick<Params, TemplatedConditional<keyof Params, never, never, keyof Params>>");

): Params => params;
AssertType(params, "Pick<Params, TemplatedConditional<keyof Params, never, never, keyof Params>>");

function fn10<T>(x: TemplatedConditional<T, never, never, T>) {
    let y: T = x;
AssertType(y, "T");
AssertType(x, "TemplatedConditional<T, never, never, T>");

    x = y;
AssertType(x = y, "T");
AssertType(x, "TemplatedConditional<T, never, never, T>");
AssertType(y, "T");
}

const fn11 = <Params>(
AssertType(fn11, "<Params>(Pick<Params, TemplatedConditional<keyof Params, keyof Params, keyof Params, never>>) => Params");
AssertType(<Params>(    params: Pick<Params, TemplatedConditional<keyof Params, keyof Params, keyof Params, never>>,): Params => params, "<Params>(Pick<Params, TemplatedConditional<keyof Params, keyof Params, keyof Params, never>>) => Params");

    params: Pick<Params, TemplatedConditional<keyof Params, keyof Params, keyof Params, never>>,
AssertType(params, "Pick<Params, TemplatedConditional<keyof Params, keyof Params, keyof Params, never>>");

): Params => params;
AssertType(params, "Pick<Params, TemplatedConditional<keyof Params, keyof Params, keyof Params, never>>");

function fn12<T>(x: TemplatedConditional<T, T, T, never>) {
    let y: T = x;
AssertType(y, "T");
AssertType(x, "TemplatedConditional<T, T, T, never>");

    x = y;
AssertType(x = y, "T");
AssertType(x, "TemplatedConditional<T, T, T, never>");
AssertType(y, "T");
}

declare let z: any;
AssertType(z, "any");

const zee = z!!!; // since x is `any`, `x extends null | undefined` should be both true and false - and thus yield `any` 
AssertType(zee, "any");
AssertType(z!!!, "any");
AssertType(z!!, "any");
AssertType(z!, "any");
AssertType(z, "any");


