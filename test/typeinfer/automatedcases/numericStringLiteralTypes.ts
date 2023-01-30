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

// === tests/cases/conformance/types/literal/numericStringLiteralTypes.ts ===
declare function AssertType(value:any, type:string):void;
type T0 = string & `${string}`;  // string
type T1 = string & `${number}`;  // `${number}
type T2 = string & `${bigint}`;  // `${bigint}
type T3<T extends string> = string & `${T}`;  // `${T}
type T4<T extends string> = string & `${Capitalize<`${T}`>}`;  // `${Capitalize<T>}`

function f1(a: boolean[], x: `${number}`) {
    let s = a[x];  // boolean
AssertType(s, "boolean");
AssertType(a[x], "boolean");
AssertType(a, "boolean[]");
AssertType(x, "`${number}`");
}

function f2(a: boolean[], x: number | `${number}`) {
    let s = a[x];  // boolean
AssertType(s, "boolean");
AssertType(a[x], "boolean");
AssertType(a, "boolean[]");
AssertType(x, "union");
}

type T10 = boolean[][`${number}`];  // boolean
type T11 = boolean[][number | `${number}`];  // boolean

type T20<T extends number | `${number}`> = T;
type T21<T extends unknown[]> = { [K in keyof T]: T20<K> };

type Container<T> = {
    value: T
}

type UnwrapContainers<T extends Container<unknown>[]> = { [K in keyof T]: T[K]['value'] };

declare function createContainer<T extends unknown>(value: T): Container<T>;

declare function f<T extends Container<unknown>[]>(containers: [...T], callback: (...values: UnwrapContainers<T>) => void): void;

const container1 = createContainer('hi')
AssertType(container1, "Container<string>");
AssertType(createContainer('hi'), "Container<string>");
AssertType(createContainer, "<T extends unknown>(T) => Container<T>");
AssertType('hi', "string");

const container2 = createContainer(2)
AssertType(container2, "Container<number>");
AssertType(createContainer(2), "Container<number>");
AssertType(createContainer, "<T extends unknown>(T) => Container<T>");
AssertType(2, "int");

f([container1, container2], (value1, value2) => {
AssertType(f([container1, container2], (value1, value2) => {    value1;  // string    value2;  // number}), "void");
AssertType(f, "<T extends Container<unknown>[]>([...T], (...UnwrapContainers<T>) => void) => void");
AssertType([container1, container2], "[Container<string>, Container<number>]");
AssertType(container1, "Container<string>");
AssertType(container2, "Container<number>");
AssertType((value1, value2) => {    value1;  // string    value2;  // number}, "(string, number) => void");
AssertType(value1, "string");
AssertType(value2, "number");

    value1;  // string
AssertType(value1, "string");

    value2;  // number
AssertType(value2, "number");

});


