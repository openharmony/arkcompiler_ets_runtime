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

// === tests/cases/compiler/ipromise2.ts ===
declare function AssertType(value:any, type:string):void;
declare module Windows.Foundation {
    export interface IPromise<T> {
        then<U>(success?: (value: T) => IPromise<U>, error?: (error: any) => IPromise<U>, progress?: (progress: any) => void ): Windows.Foundation.IPromise<U>;
        then<U>(success?: (value: T) => IPromise<U>, error?: (error: any) => U, progress?: (progress: any) => void ): Windows.Foundation.IPromise<U>;
        then<U>(success?: (value: T) => U, error?: (error: any) => IPromise<U>, progress?: (progress: any) => void ): Windows.Foundation.IPromise<U>;
        then<U>(success?: (value: T) => U, error?: (error: any) => U, progress?: (progress: any) => void ): Windows.Foundation.IPromise<U>;
        done<U>(success?: (value: T) => any, error?: (error: any) => any, progress?: (progress: any) => void ): void;
        value: T;
    }
}

let p: Windows.Foundation.IPromise<string>;
AssertType(p, "Windows.Foundation.IPromise<string>");
AssertType(Windows, "any");
AssertType(Foundation, "any");

let p2 = p.then(function (s) {
AssertType(p2, "Windows.Foundation.IPromise<number>");
AssertType(p.then, "{ <U>(?(string) => Windows.Foundation.IPromise<U>, ?(any) => Windows.Foundation.IPromise<U>, ?(any) => void): Windows.Foundation.IPromise<U>; <U>(?(string) => Windows.Foundation.IPromise<U>, ?(any) => U, ?(any) => void): Windows.Foundation.IPromise<U>; <U>(?(string) => U, ?(any) => Windows.Foundation.IPromise<U>, ?(any) => void): Windows.Foundation.IPromise<U>; <U>(?(string) => U, ?(any) => U, ?(any) => void): Windows.Foundation.IPromise<U>; }");
AssertType(function (s) {    return 34;}, "(string) => number");
AssertType(s, "string");
AssertType(p.then(function (s) {    return 34;} ), "Windows.Foundation.IPromise<number>");

AssertType(34, "int");
    return 34;

} );


let x: number = p2.value;
AssertType(x, "number");
AssertType(p2.value, "number");



