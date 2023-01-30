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

// === tests/cases/compiler/typeParameterExtendsPrimitive.ts ===
declare function AssertType(value:any, type:string):void;
// #14473
function f<T extends number>() {
    let t: T;
AssertType(t, "T");

    let v = {
AssertType(v, "{ [number]: number; }");
AssertType({        [t]: 0    }, "{ [number]: number; }");

        [t]: 0
AssertType([t], "number");
AssertType(t, "T");
AssertType(0, "int");
    }
AssertType(t + t, "number");
AssertType(t, "T");
AssertType(t, "T");
    return t + t;
}

// #15501
interface I { x: number }
type IdMap<T> = { [P in keyof T]: T[P] };
function g<T extends I>(i: IdMap<T>) {
    const n: number = i.x;
AssertType(n, "number");
AssertType(i.x, "T["x"]");

AssertType(i.x * 2, "number");
AssertType(i.x, "T["x"]");
AssertType(2, "int");
    return i.x * 2;
}

// #17069
function h<T extends Record<K, number>, K extends string>(array: T[], prop: K): number {
    let result = 0;
AssertType(result, "number");
AssertType(0, "int");

    for (const v of array) {
AssertType(v, "T");
AssertType(array, "T[]");

        result += v[prop];
AssertType(result += v[prop], "number");
AssertType(result, "number");
AssertType(v[prop], "T[K]");
AssertType(v, "T");
AssertType(prop, "K");
    }
AssertType(result, "number");
    return result;
}


