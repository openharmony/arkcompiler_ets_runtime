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

// === tests/cases/compiler/ipromise4.ts ===
declare function AssertType(value:any, type:string):void;
declare module Windows.Foundation {
    export interface IPromise<T> {
        then<U>(success?: (value: T) => IPromise<U>, error?: (error: any) => IPromise<U>, progress?: (progress: any) => void ): Windows.Foundation.IPromise<U>;
        then<U>(success?: (value: T) => IPromise<U>, error?: (error: any) => U, progress?: (progress: any) => void ): Windows.Foundation.IPromise<U>;
        then<U>(success?: (value: T) => U, error?: (error: any) => IPromise<U>, progress?: (progress: any) => void ): Windows.Foundation.IPromise<U>;
        then<U>(success?: (value: T) => U, error?: (error: any) => U, progress?: (progress: any) => void ): Windows.Foundation.IPromise<U>;
        done? <U>(success?: (value: T) => any, error?: (error: any) => any, progress?: (progress: any) => void ): void;
    }
}
 
let p: Windows.Foundation.IPromise<number> = null;
AssertType(p, "Windows.Foundation.IPromise<number>");
AssertType(Windows, "any");
AssertType(Foundation, "any");
AssertType(null, "null");
 
p.then(function (x) { } ); // should not error
AssertType(p.then(function (x) { } ), "Windows.Foundation.IPromise<void>");
AssertType(p.then, "{ <U>(?(number) => Windows.Foundation.IPromise<U>, ?(any) => Windows.Foundation.IPromise<U>, ?(any) => void): Windows.Foundation.IPromise<U>; <U>(?(number) => Windows.Foundation.IPromise<U>, ?(any) => U, ?(any) => void): Windows.Foundation.IPromise<U>; <U>(?(number) => U, ?(any) => Windows.Foundation.IPromise<U>, ?(any) => void): Windows.Foundation.IPromise<U>; <U>(?(number) => U, ?(any) => U, ?(any) => void): Windows.Foundation.IPromise<U>; }");
AssertType(function (x) { }, "(number) => void");
AssertType(x, "number");

p.then(function (x) { 
AssertType(p.then(function (x) { return "hello"; } ).then(function (x) { return x } ), "Windows.Foundation.IPromise<string>");
AssertType(p.then(function (x) { return "hello"; } ).then, "{ <U>(?(string) => Windows.Foundation.IPromise<U>, ?(any) => Windows.Foundation.IPromise<U>, ?(any) => void): Windows.Foundation.IPromise<U>; <U>(?(string) => Windows.Foundation.IPromise<U>, ?(any) => U, ?(any) => void): Windows.Foundation.IPromise<U>; <U>(?(string) => U, ?(any) => Windows.Foundation.IPromise<U>, ?(any) => void): Windows.Foundation.IPromise<U>; <U>(?(string) => U, ?(any) => U, ?(any) => void): Windows.Foundation.IPromise<U>; }");
AssertType("hello", "string");
AssertType(function (x) { return x }, "(string) => string");
AssertType(x, "string");
AssertType(x, "string");
return "hello"; } ).then(function (x) { return x } ); // should not error
 


