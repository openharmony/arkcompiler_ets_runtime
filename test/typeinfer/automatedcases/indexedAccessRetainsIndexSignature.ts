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

// === tests/cases/compiler/indexedAccessRetainsIndexSignature.ts ===
declare function AssertType(value:any, type:string):void;
type Diff<T extends keyof any, U extends keyof any> =
    ({ [P in T]: P } & { [P in U]: never } & { [x: string]: never })[T]
type Omit<U, K extends keyof U> = Pick<U, Diff<keyof U, K>>
type Omit1<U, K extends keyof U> = Pick<U, Diff<keyof U, K>>;
// is in fact an equivalent of

type Omit2<T, K extends keyof T> = {[P in Diff<keyof T, K>]: T[P]};

type O = Omit<{ a: number, b: string }, 'a'>
export const o: O = { b: '' 
AssertType(o, "O");

AssertType({ b: '' }, "{ b: string; }");

AssertType(b, "string");

AssertType('', "string");
}


