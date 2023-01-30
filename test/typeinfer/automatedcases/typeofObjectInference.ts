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

// === tests/cases/compiler/typeofObjectInference.ts ===
declare function AssertType(value:any, type:string):void;
let val = 1
AssertType(val, "number");
AssertType(1, "int");

function decorateA<O extends any>(fn: (first: {value: typeof val}) => O) {
AssertType((): O => fn({value: val}), "() => O");
AssertType(fn({value: val}), "O");
AssertType(fn, "({ value: number; }) => O");
AssertType({value: val}, "{ value: number; }");
AssertType(value, "number");
AssertType(val, "number");
    return (): O => fn({value: val})
}
let a = decorateA(({value}) => 5)
AssertType(a, "() => number");
AssertType(decorateA(({value}) => 5), "() => number");
AssertType(decorateA, "<O extends unknown>(({ value: number; }) => O) => () => O");
AssertType(({value}) => 5, "({ value: number; }) => number");
AssertType(value, "number");
AssertType(5, "int");

function decorateB<O extends any>(fn: (first: typeof val) => O) {
AssertType((): O => fn(val), "() => O");
AssertType(fn(val), "O");
AssertType(fn, "(number) => O");
AssertType(val, "number");
    return (): O => fn(val)
}
let b = decorateB((value) => 5)
AssertType(b, "() => number");
AssertType(decorateB((value) => 5), "() => number");
AssertType(decorateB, "<O extends unknown>((number) => O) => () => O");
AssertType((value) => 5, "(number) => number");
AssertType(value, "number");
AssertType(5, "int");

function decorateC<O extends any>(fn: (first: {value: number}) => O) {
AssertType((): O => fn({value: val}), "() => O");
AssertType(fn({value: val}), "O");
AssertType(fn, "({ value: number; }) => O");
AssertType({value: val}, "{ value: number; }");
AssertType(value, "number");
AssertType(val, "number");
    return (): O => fn({value: val})
}
let c = decorateC(({value}) => 5)
AssertType(c, "() => number");
AssertType(decorateC(({value}) => 5), "() => number");
AssertType(decorateC, "<O extends unknown>(({ value: number; }) => O) => () => O");
AssertType(({value}) => 5, "({ value: number; }) => number");
AssertType(value, "number");
AssertType(5, "int");

type First = {value: typeof val}
function decorateD<O extends any>(fn: (first: First) => O) {
AssertType((): O => fn({value: val}), "() => O");
AssertType(fn({value: val}), "O");
AssertType(fn, "(First) => O");
AssertType({value: val}, "{ value: number; }");
AssertType(value, "number");
AssertType(val, "number");
    return (): O => fn({value: val})
}
let d = decorateD(({value}) => 5)
AssertType(d, "() => number");
AssertType(decorateD(({value}) => 5), "() => number");
AssertType(decorateD, "<O extends unknown>((First) => O) => () => O");
AssertType(({value}) => 5, "(First) => number");
AssertType(value, "number");
AssertType(5, "int");


