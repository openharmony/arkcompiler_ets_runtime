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

// === tests/cases/compiler/checkInfiniteExpansionTermination.ts ===
declare function AssertType(value:any, type:string):void;
// Regression test for #1002
// Before fix this code would cause infinite loop

interface IObservable<T> {
    n: IObservable<T[]>; // Needed, must be T[]
}

// Needed
interface ISubject<T> extends IObservable<T> { }

interface Foo { x }
interface Bar { y }

let values: IObservable<Foo>;
AssertType(values, "IObservable<Foo>");

let values2: ISubject<Bar>;
AssertType(values2, "ISubject<Bar>");

values = values2;
AssertType(values = values2, "ISubject<Bar>");
AssertType(values, "IObservable<Foo>");
AssertType(values2, "ISubject<Bar>");


