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

// === tests/cases/conformance/types/typeRelationships/widenedTypes/arrayLiteralWidened.ts ===
declare function AssertType(value:any, type:string):void;
// array literals are widened upon assignment according to their element type

let a = []; // any[]
AssertType(a, "any[]");
AssertType([], "undefined[]");

let a = [,,];
AssertType(a, "any[]");
AssertType([,,], "undefined[]");
AssertType(, "undefined");
AssertType(, "undefined");

let a = [null, null];
AssertType(a, "any[]");
AssertType([null, null], "null[]");
AssertType(null, "null");
AssertType(null, "null");

let a = [undefined, undefined];
AssertType(a, "any[]");
AssertType([undefined, undefined], "undefined[]");
AssertType(undefined, "undefined");
AssertType(undefined, "undefined");

let b = [[], [null, null]]; // any[][]
AssertType(b, "any[][]");
AssertType([[], [null, null]], "undefined[][]");
AssertType([], "undefined[]");
AssertType([null, null], "null[]");
AssertType(null, "null");
AssertType(null, "null");

let b = [[], []];
AssertType(b, "any[][]");
AssertType([[], []], "undefined[][]");
AssertType([], "undefined[]");
AssertType([], "undefined[]");

let b = [[undefined, undefined]];
AssertType(b, "any[][]");
AssertType([[undefined, undefined]], "undefined[][]");
AssertType([undefined, undefined], "undefined[]");
AssertType(undefined, "undefined");
AssertType(undefined, "undefined");

let c = [[[]]]; // any[][][]
AssertType(c, "any[][][]");
AssertType([[[]]], "undefined[][][]");
AssertType([[]], "undefined[][]");
AssertType([], "undefined[]");

let c = [[[null]],[undefined]]
AssertType(c, "any[][][]");
AssertType([[[null]],[undefined]], "null[][][]");
AssertType([[null]], "null[][]");
AssertType([null], "null[]");
AssertType(null, "null");
AssertType([undefined], "undefined[]");
AssertType(undefined, "undefined");

// no widening when one or more elements are non-widening

let x: undefined = undefined;
AssertType(x, "undefined");
AssertType(undefined, "undefined");

let d = [x];
AssertType(d, "undefined[]");
AssertType([x], "undefined[]");
AssertType(x, "undefined");

let d = [, x];
AssertType(d, "undefined[]");
AssertType([, x], "undefined[]");
AssertType(, "undefined");
AssertType(x, "undefined");

let d = [undefined, x];
AssertType(d, "undefined[]");
AssertType([undefined, x], "undefined[]");
AssertType(undefined, "undefined");
AssertType(x, "undefined");


