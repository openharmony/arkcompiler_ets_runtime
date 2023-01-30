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

// === tests/cases/compiler/propagationOfPromiseInitialization.ts ===
declare function AssertType(value:any, type:string):void;
interface IPromise<T> {
    then<TResult>(successCallback: (promiseValue: T) => TResult, errorCallback?: (reason: any) => TResult): IPromise<TResult>;
}

let foo: IPromise<number>;
AssertType(foo, "IPromise<number>");

foo.then((x) => {
AssertType(foo.then((x) => {    // x is inferred to be a number    return "asdf";}).then, "<TResult>((string) => TResult, ?(any) => TResult) => IPromise<TResult>");
AssertType(foo.then((x) => {    // x is inferred to be a number    return "asdf";}).then((x) => {    // x is inferred to be string    x.length;    return 123;}), "IPromise<number>");

    // x is inferred to be a number
AssertType("asdf", "string");
    return "asdf";

}).then((x) => {
AssertType(x, "string");
AssertType((x) => {    // x is inferred to be string    x.length;    return 123;}, "(string) => number");

    // x is inferred to be string
    x.length;
AssertType(x.length, "number");

AssertType(123, "int");
    return 123;

});


