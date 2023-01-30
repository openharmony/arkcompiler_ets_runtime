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

// === tests/cases/compiler/duplicateOverloadInTypeAugmentation1.ts ===
declare function AssertType(value:any, type:string):void;
interface Array<T> {
    reduce(callbackfn: (previousValue: T, currentValue: T, currentIndex: number, array: T[]) => T,
        initialValue?: T): T;
    reduce<U>(callbackfn: (previousValue: U, currentValue: T, currentIndex: number, array: T[]) => U,
        initialValue: U): U;
}
let a: Array<string>;
AssertType(a, "string[]");

let r5 = a.reduce((x, y) => x + y);
AssertType(r5, "string");
AssertType(a.reduce((x, y) => x + y), "string");
AssertType(a.reduce, "{ ((string, string, number, string[]) => string): string; ((string, string, number, string[]) => string, string): string; <U>((U, string, number, string[]) => U, U): U; ((string, string, number, string[]) => string, ?string): string; <U>((U, string, number, string[]) => U, U): U; }");
AssertType((x, y) => x + y, "(string, string) => string");
AssertType(x, "string");
AssertType(y, "string");
AssertType(x + y, "string");
AssertType(x, "string");
AssertType(y, "string");


