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

// === tests/cases/compiler/promiseTypeInference.ts ===
declare function AssertType(value:any, type:string):void;
declare class CPromise<T> {
    then<U>(success?: (value: T) => CPromise<U>): CPromise<U>;
}
interface IPromise<T> {
    then<U>(success?: (value: T) => IPromise<U>): IPromise<U>;
}
declare function load(name: string): CPromise<string>;
declare function convert(s: string): IPromise<number>;

let $$x = load("something").then(s => convert(s));
AssertType($$x, "CPromise<number>");
AssertType(load("something").then(s => convert(s)), "CPromise<number>");
AssertType(load("something").then, "<U>(?(string) => CPromise<U>) => CPromise<U>");
AssertType(s => convert(s), "(string) => IPromise<number>");
AssertType(s, "string");
AssertType(convert(s), "IPromise<number>");
AssertType(convert, "(string) => IPromise<number>");
AssertType(s, "string");


