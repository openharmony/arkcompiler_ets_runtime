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

// === tests/cases/compiler/mappedTypeUnionConstraintInferences.ts ===
declare function AssertType(value:any, type:string):void;
export declare type Omit<T, K extends keyof T> = Pick<T, Exclude<keyof T, K>>;
export declare type PartialProperties<T, K extends keyof T> = Partial<Pick<T, K>> & Omit<T, K>;
export function doSomething_Actual<T extends {
    prop: string;
}>(a: T) {
    const x: { [P in keyof PartialProperties<T, "prop">]: PartialProperties<T, "prop">[P]; } = null as any;
AssertType(x, "{ [P in keyof PartialProperties<T, "prop">]: PartialProperties<T, "prop">[P]; }");
AssertType(null as any, "any");
AssertType(null, "null");

AssertType(x, "{ [P in keyof PartialProperties<T, "prop">]: PartialProperties<T, "prop">[P]; }");
    return x;
}
export declare function doSomething_Expected<T extends {
    prop: string;
}>(a: T): { [P in keyof PartialProperties<T, "prop">]: PartialProperties<T, "prop">[P]; };

export let a = doSomething_Actual({ prop: "test" });
AssertType(a, "{ prop?: string; }");
AssertType(doSomething_Actual({ prop: "test" }), "{ prop?: string; }");
AssertType(doSomething_Actual, "<T extends { prop: string; }>(T) => { [P in keyof PartialProperties<T, "prop">]: PartialProperties<T, "prop">[P]; }");
AssertType({ prop: "test" }, "{ prop: string; }");
AssertType(prop, "string");
AssertType("test", "string");

a = {} // should be fine, equivalent to below
AssertType(a = {}, "{}");
AssertType(a, "{ prop?: string; }");
AssertType({}, "{}");

export let b = doSomething_Expected({ prop: "test" });
AssertType(b, "{ prop?: string; }");
AssertType(doSomething_Expected({ prop: "test" }), "{ prop?: string; }");
AssertType(doSomething_Expected, "<T extends { prop: string; }>(T) => { [P in keyof PartialProperties<T, "prop">]: PartialProperties<T, "prop">[P]; }");
AssertType({ prop: "test" }, "{ prop: string; }");
AssertType(prop, "string");
AssertType("test", "string");

b = {} // fine
AssertType(b = {}, "{}");
AssertType(b, "{ prop?: string; }");
AssertType({}, "{}");


