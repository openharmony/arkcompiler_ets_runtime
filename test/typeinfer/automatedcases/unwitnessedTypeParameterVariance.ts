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

// === tests/cases/compiler/unwitnessedTypeParameterVariance.ts ===
declare function AssertType(value:any, type:string):void;
// Repros from #33872

export interface CalcObj<O> {
    read: (origin: O) => CalcValue<O>;
}

export type CalcValue<O> = CalcObj<O>;

function foo<O>() {
    const unk: CalcObj<unknown> = { read: (origin: unknown) => unk 
AssertType(unk, "CalcObj<unknown>");

AssertType({ read: (origin: unknown) => unk }, "{ read: (unknown) => CalcObj<unknown>; }");

AssertType(read, "(unknown) => CalcObj<unknown>");

AssertType((origin: unknown) => unk, "(unknown) => CalcObj<unknown>");

AssertType(origin, "unknown");

AssertType(unk, "CalcObj<unknown>");
}

    const x: CalcObj<O> = unk;
AssertType(x, "CalcObj<O>");
AssertType(unk, "CalcObj<unknown>");
}

type A<T> = B<T>;

interface B<T> {
    prop: A<T>;
}

declare let a: A<number>;
AssertType(a, "A<number>");

declare let b: A<3>;
AssertType(b, "A<3>");
 
b = a;
AssertType(b = a, "A<number>");
AssertType(b, "A<3>");
AssertType(a, "A<number>");


