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

// === tests/cases/compiler/thisInTupleTypeParameterConstraints.ts ===
declare function AssertType(value:any, type:string):void;
/// <reference no-default-lib="true"/>

interface Boolean {}
interface IArguments {}
interface Function {}
interface Number {}
interface RegExp {}
interface Object {}
interface String {}

interface Array<T> {
    // 4 methods will run out of memory if this-types are not instantiated
    // correctly for tuple types that are type parameter constraints
    map<U>(arg: this): void;
    reduceRight<U>(arg: this): void;
    reduce<U>(arg: this): void;
    reduce2<U>(arg: this): void;
}

declare function f<T extends [(x: number) => number]>(a: T): void;
let x: [(x: number) => number];
AssertType(x, "[(number) => number]");
AssertType(x, "number");

f(x);
AssertType(f(x), "void");
AssertType(f, "<T extends [(number) => number]>(T) => void");
AssertType(x, "[(number) => number]");


