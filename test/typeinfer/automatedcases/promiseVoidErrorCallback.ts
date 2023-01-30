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

// === tests/cases/compiler/promiseVoidErrorCallback.ts ===
declare function AssertType(value:any, type:string):void;
interface T1 {
    __t1: string;
}

interface T2 {
    __t2: string;
}

interface T3 {
    __t3: string;
}

function f1(): Promise<T1> {
AssertType(Promise.resolve({ __t1: "foo_t1" }), "Promise<{ __t1: string; }>");
AssertType(Promise.resolve, "{ (): Promise<void>; <T>(T): Promise<Awaited<T>>; <T>(union): Promise<Awaited<T>>; }");
AssertType({ __t1: "foo_t1" }, "{ __t1: string; }");
AssertType(__t1, "string");
AssertType("foo_t1", "string");
    return Promise.resolve({ __t1: "foo_t1" });
}

function f2(x: T1): T2 {
AssertType({ __t2: x.__t1 + ":foo_21" }, "{ __t2: string; }");
AssertType(__t2, "string");
AssertType(x.__t1 + ":foo_21", "string");
AssertType(x.__t1, "string");
AssertType(":foo_21", "string");
    return { __t2: x.__t1 + ":foo_21" };
}

let x3 = f1()
AssertType(x3, "Promise<{ __t3: string; }>");
AssertType(f1()    .then(f2, (e: Error) => {    throw e;})    .then, "<TResult1 = T2, TResult2 = never>(?(T2) => union, ?(any) => union) => Promise<union>");
AssertType(f1()    .then(f2, (e: Error) => {    throw e;})    .then((x: T2) => {    return { __t3: x.__t2 + "bar" };}), "Promise<{ __t3: string; }>");

    .then(f2, (e: Error) => {
    throw e;
AssertType(e, "Error");

})
    .then((x: T2) => {
AssertType(x, "T2");
AssertType((x: T2) => {    return { __t3: x.__t2 + "bar" };}, "(T2) => { __t3: string; }");

AssertType({ __t3: x.__t2 + "bar" }, "{ __t3: string; }");
AssertType(__t3, "string");
AssertType(x.__t2 + "bar", "string");
AssertType(x.__t2, "string");
AssertType("bar", "string");
    return { __t3: x.__t2 + "bar" };

});

