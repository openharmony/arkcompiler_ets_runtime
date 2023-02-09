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

// === tests/cases/compiler/tupleTypeInference.ts ===
declare function AssertType(value:any, type:string):void;
declare let $q: IQService;
AssertType($q, "IQService");

interface IQService {
    all<T1, T2, T3>(x: [IPromise<T1>, IPromise<T2>, IPromise<T3>]): IPromise<[T1, T2, T3]>;
    all<T1, T2>(x: [IPromise<T1>, IPromise<T2>]): IPromise<[T1, T2]>;
    all<T1>(x: [IPromise<T1>]): IPromise<[T1]>;
    when<T>(t?: T): IPromise<T>;
}

interface IPromise<T> {
    then<TResult>(callback: (t: T) => TResult): IPromise<TResult>;
}

// Implicit different types
let a = $q.all([$q.when<string>(), $q.when<number>()]);
AssertType(a, "IPromise<[string, number]>");
AssertType($q.all([$q.when<string>(), $q.when<number>()]), "IPromise<[string, number]>");
AssertType($q.all, "{ <T1, T2, T3>([IPromise<T1>, IPromise<T2>, IPromise<T3>]): IPromise<[T1, T2, T3]>; <T1, T2>([IPromise<T1>, IPromise<T2>]): IPromise<[T1, T2]>; <T1>([IPromise<T1>]): IPromise<[T1]>; }");
AssertType([$q.when<string>(), $q.when<number>()], "[IPromise<string>, IPromise<number>]");
AssertType($q.when<string>(), "IPromise<string>");
AssertType($q.when, "<T>(?T) => IPromise<T>");
AssertType($q.when<number>(), "IPromise<number>");
AssertType($q.when, "<T>(?T) => IPromise<T>");

// Explicit different types
let b = $q.all<string, number>([$q.when<string>(), $q.when<number>()]);
AssertType(b, "IPromise<[string, number]>");
AssertType($q.all<string, number>([$q.when<string>(), $q.when<number>()]), "IPromise<[string, number]>");
AssertType($q.all, "{ <T1, T2, T3>([IPromise<T1>, IPromise<T2>, IPromise<T3>]): IPromise<[T1, T2, T3]>; <T1, T2>([IPromise<T1>, IPromise<T2>]): IPromise<[T1, T2]>; <T1>([IPromise<T1>]): IPromise<[T1]>; }");
AssertType([$q.when<string>(), $q.when<number>()], "[IPromise<string>, IPromise<number>]");
AssertType($q.when<string>(), "IPromise<string>");
AssertType($q.when, "<T>(?T) => IPromise<T>");
AssertType($q.when<number>(), "IPromise<number>");
AssertType($q.when, "<T>(?T) => IPromise<T>");

// Implicit identical types
let c = $q.all([$q.when<string>(), $q.when<string>()]);
AssertType(c, "IPromise<[string, string]>");
AssertType($q.all([$q.when<string>(), $q.when<string>()]), "IPromise<[string, string]>");
AssertType($q.all, "{ <T1, T2, T3>([IPromise<T1>, IPromise<T2>, IPromise<T3>]): IPromise<[T1, T2, T3]>; <T1, T2>([IPromise<T1>, IPromise<T2>]): IPromise<[T1, T2]>; <T1>([IPromise<T1>]): IPromise<[T1]>; }");
AssertType([$q.when<string>(), $q.when<string>()], "[IPromise<string>, IPromise<string>]");
AssertType($q.when<string>(), "IPromise<string>");
AssertType($q.when, "<T>(?T) => IPromise<T>");
AssertType($q.when<string>(), "IPromise<string>");
AssertType($q.when, "<T>(?T) => IPromise<T>");


