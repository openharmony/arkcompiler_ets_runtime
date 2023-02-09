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

// === tests/cases/conformance/es7/exponentiationOperator/emitCompoundExponentiationAssignmentWithIndexingOnLHS1.ts ===
declare function AssertType(value:any, type:string):void;
let array0 = [1, 2, 3]
AssertType(array0, "number[]");
AssertType([1, 2, 3], "number[]");
AssertType(1, "int");
AssertType(2, "int");
AssertType(3, "int");

let i0 = 0;
AssertType(i0, "number");
AssertType(0, "int");

array0[++i0] **= 2;
AssertType(array0[++i0] **= 2, "number");
AssertType(array0[++i0], "number");
AssertType(array0, "number[]");
AssertType(++i0, "number");
AssertType(i0, "number");
AssertType(2, "int");

let array1 = [1, 2, 3]
AssertType(array1, "number[]");
AssertType([1, 2, 3], "number[]");
AssertType(1, "int");
AssertType(2, "int");
AssertType(3, "int");

let i1 = 0;
AssertType(i1, "number");
AssertType(0, "int");

array1[++i1] **= array1[++i1] **= 2;
AssertType(array1[++i1] **= array1[++i1] **= 2, "number");
AssertType(array1[++i1], "number");
AssertType(array1, "number[]");
AssertType(++i1, "number");
AssertType(i1, "number");
AssertType(array1[++i1] **= 2, "number");
AssertType(array1[++i1], "number");
AssertType(array1, "number[]");
AssertType(++i1, "number");
AssertType(i1, "number");
AssertType(2, "int");

let array2 = [1, 2, 3]
AssertType(array2, "number[]");
AssertType([1, 2, 3], "number[]");
AssertType(1, "int");
AssertType(2, "int");
AssertType(3, "int");

let i2 = 0;
AssertType(i2, "number");
AssertType(0, "int");

array2[++i2] **= array2[++i2] ** 2;
AssertType(array2[++i2] **= array2[++i2] ** 2, "number");
AssertType(array2[++i2], "number");
AssertType(array2, "number[]");
AssertType(++i2, "number");
AssertType(i2, "number");
AssertType(array2[++i2] ** 2, "number");
AssertType(array2[++i2], "number");
AssertType(array2, "number[]");
AssertType(++i2, "number");
AssertType(i2, "number");
AssertType(2, "int");

let array3 = [2, 2, 3];
AssertType(array3, "number[]");
AssertType([2, 2, 3], "number[]");
AssertType(2, "int");
AssertType(2, "int");
AssertType(3, "int");

let j0 = 0, j1 = 1;
AssertType(j0, "number");
AssertType(0, "int");
AssertType(j1, "number");
AssertType(1, "int");

array3[j0++] **= array3[j1++] **= array3[j0++] **= 1;
AssertType(array3[j0++] **= array3[j1++] **= array3[j0++] **= 1, "number");
AssertType(array3[j0++], "number");
AssertType(array3, "number[]");
AssertType(j0++, "number");
AssertType(j0, "number");
AssertType(array3[j1++] **= array3[j0++] **= 1, "number");
AssertType(array3[j1++], "number");
AssertType(array3, "number[]");
AssertType(j1++, "number");
AssertType(j1, "number");
AssertType(array3[j0++] **= 1, "number");
AssertType(array3[j0++], "number");
AssertType(array3, "number[]");
AssertType(j0++, "number");
AssertType(j0, "number");
AssertType(1, "int");


