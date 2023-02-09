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

// === tests/cases/compiler/asyncFunctionsAndStrictNullChecks.ts ===
declare function AssertType(value:any, type:string):void;
declare namespace Windows.Foundation {
    interface IPromise<TResult> {
        then<U>(success?: (value: TResult) => IPromise<U>, error?: (error: any) => IPromise<U>, progress?: (progress: any) => void): IPromise<U>;
        then<U>(success?: (value: TResult) => IPromise<U>, error?: (error: any) => U, progress?: (progress: any) => void): IPromise<U>;
        then<U>(success?: (value: TResult) => U, error?: (error: any) => IPromise<U>, progress?: (progress: any) => void): IPromise<U>;
        then<U>(success?: (value: TResult) => U, error?: (error: any) => U, progress?: (progress: any) => void): IPromise<U>;
        done<U>(success?: (value: TResult) => any, error?: (error: any) => any, progress?: (progress: any) => void): void;

        cancel(): void;
    }
}

async function sample(promise: Windows.Foundation.IPromise<number>) {
    let number = await promise;
AssertType(number, "number");
AssertType(await promise, "number");
AssertType(promise, "Windows.Foundation.IPromise<number>");
}


declare function resolve1<T>(value: T): Promise<T>;
declare function resolve2<T>(value: T): Windows.Foundation.IPromise<T>;

async function sample2(x?: number) {
    let x1 = await resolve1(x);
AssertType(x1, "union");
AssertType(await resolve1(x), "union");
AssertType(resolve1(x), "Promise<union>");
AssertType(resolve1, "<T>(T) => Promise<T>");
AssertType(x, "union");

    let x2 = await resolve2(x);
AssertType(x2, "union");
AssertType(await resolve2(x), "union");
AssertType(resolve2(x), "Windows.Foundation.IPromise<union>");
AssertType(resolve2, "<T>(T) => Windows.Foundation.IPromise<T>");
AssertType(x, "union");
}


