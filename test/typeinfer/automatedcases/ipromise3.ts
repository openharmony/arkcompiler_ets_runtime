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

// === tests/cases/compiler/ipromise3.ts ===
declare function AssertType(value:any, type:string):void;
interface IPromise3<T> {
    then<U>(success?: (value: T) => IPromise3<U>, error?: (error: any) => IPromise3<U>, progress?: (progress: any) => void ): IPromise3<U>;
    then<U>(success?: (value: T) => IPromise3<U>, error?: (error: any) => U, progress?: (progress: any) => void ): IPromise3<U>;
    then<U>(success?: (value: T) => U, error?: (error: any) => IPromise3<U>, progress?: (progress: any) => void ): IPromise3<U>;
    then<U>(success?: (value: T) => U, error?: (error: any) => U, progress?: (progress: any) => void ): IPromise3<U>;
    done? <U>(success?: (value: T) => any, error?: (error: any) => any, progress?: (progress: any) => void ): void;
}
let p1: IPromise3<string>;
AssertType(p1, "IPromise3<string>");

let p2: IPromise3<string> = p1.then(function (x) {
AssertType(p2, "IPromise3<string>");
AssertType(p1.then, "{ <U>(?(string) => IPromise3<U>, ?(any) => IPromise3<U>, ?(any) => void): IPromise3<U>; <U>(?(string) => IPromise3<U>, ?(any) => U, ?(any) => void): IPromise3<U>; <U>(?(string) => U, ?(any) => IPromise3<U>, ?(any) => void): IPromise3<U>; <U>(?(string) => U, ?(any) => U, ?(any) => void): IPromise3<U>; }");
AssertType(function (x) {    return x;}, "(string) => string");
AssertType(x, "string");
AssertType(p1.then(function (x) {    return x;}), "IPromise3<string>");

AssertType(x, "string");
    return x;

});


