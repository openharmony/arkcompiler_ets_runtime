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

// === tests/cases/conformance/types/tuple/typeInferenceWithTupleType.ts ===
declare function AssertType(value:any, type:string):void;
function combine<T, U>(x: T, y: U): [T, U] {
AssertType([x, y], "[T, U]");
AssertType(x, "T");
AssertType(y, "U");
    return [x, y];
}

let combineResult = combine("string", 10);
AssertType(combineResult, "[string, number]");
AssertType(combine("string", 10), "[string, number]");
AssertType(combine, "<T, U>(T, U) => [T, U]");
AssertType("string", "string");
AssertType(10, "int");

let combineEle1 = combineResult[0]; // string
AssertType(combineEle1, "string");
AssertType(combineResult[0], "string");
AssertType(combineResult, "[string, number]");
AssertType(0, "int");

let combineEle2 = combineResult[1]; // number
AssertType(combineEle2, "number");
AssertType(combineResult[1], "number");
AssertType(combineResult, "[string, number]");
AssertType(1, "int");

function zip<T, U>(array1: T[], array2: U[]): [[T, U]] {
    if (array1.length != array2.length) {
AssertType(array1.length != array2.length, "boolean");
AssertType(array1.length, "number");
AssertType(array2.length, "number");

AssertType([[undefined, undefined]], "[[undefined, undefined]]");
AssertType([undefined, undefined], "[undefined, undefined]");
AssertType(undefined, "undefined");
AssertType(undefined, "undefined");
        return [[undefined, undefined]];
    }
    let length = array1.length;
AssertType(length, "number");
AssertType(array1.length, "number");

    let zipResult: [[T, U]];
AssertType(zipResult, "[[T, U]]");

    for (let i = 0; i < length; ++i) {
AssertType(i, "number");
AssertType(0, "int");
AssertType(i < length, "boolean");
AssertType(i, "number");
AssertType(length, "number");
AssertType(++i, "number");
AssertType(i, "number");

        zipResult.push([array1[i], array2[i]]);
AssertType(zipResult.push([array1[i], array2[i]]), "number");
AssertType(zipResult.push, "(...[T, U][]) => number");
AssertType([array1[i], array2[i]], "[T, U]");
AssertType(array1[i], "T");
AssertType(array1, "T[]");
AssertType(i, "number");
AssertType(array2[i], "U");
AssertType(array2, "U[]");
AssertType(i, "number");
    }
AssertType(zipResult, "[[T, U]]");
    return zipResult;
}

let zipResult = zip(["foo", "bar"], [5, 6]);
AssertType(zipResult, "[[string, number]]");
AssertType(zip(["foo", "bar"], [5, 6]), "[[string, number]]");
AssertType(zip, "<T, U>(T[], U[]) => [[T, U]]");
AssertType(["foo", "bar"], "string[]");
AssertType("foo", "string");
AssertType("bar", "string");
AssertType([5, 6], "number[]");
AssertType(5, "int");
AssertType(6, "int");

let zipResultEle = zipResult[0]; // [string, number]
AssertType(zipResultEle, "[string, number]");
AssertType(zipResult[0], "[string, number]");
AssertType(zipResult, "[[string, number]]");
AssertType(0, "int");

let zipResultEleEle = zipResult[0][0]; // string
AssertType(zipResultEleEle, "string");
AssertType(zipResult[0][0], "string");
AssertType(zipResult[0], "[string, number]");
AssertType(zipResult, "[[string, number]]");
AssertType(0, "int");
AssertType(0, "int");

// #33559 and #33752

declare function f1<T1, T2>(values: [T1[], T2[]]): T1;
declare function f2<T1, T2>(values: readonly [T1[], T2[]]): T1;

let expected: "a";
AssertType(expected, "string");

expected = f1(undefined as ["a"[], "b"[]]);
AssertType(expected = f1(undefined as ["a"[], "b"[]]), "string");
AssertType(expected, "string");
AssertType(f1(undefined as ["a"[], "b"[]]), "string");
AssertType(f1, "<T1, T2>([T1[], T2[]]) => T1");
AssertType(undefined as ["a"[], "b"[]], "["a"[], "b"[]]");
AssertType(undefined, "undefined");

expected = f2(undefined as ["a"[], "b"[]]);
AssertType(expected = f2(undefined as ["a"[], "b"[]]), "string");
AssertType(expected, "string");
AssertType(f2(undefined as ["a"[], "b"[]]), "string");
AssertType(f2, "<T1, T2>(readonly [T1[], T2[]]) => T1");
AssertType(undefined as ["a"[], "b"[]], "["a"[], "b"[]]");
AssertType(undefined, "undefined");


