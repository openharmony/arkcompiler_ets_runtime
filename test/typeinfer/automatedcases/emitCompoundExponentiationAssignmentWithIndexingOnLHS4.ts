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

// === tests/cases/conformance/es7/exponentiationOperator/emitCompoundExponentiationAssignmentWithIndexingOnLHS4.ts ===
declare function AssertType(value:any, type:string):void;
let globalCounter = 0;
AssertType(globalCounter, "number");
AssertType(0, "int");

function incrementIdx(max: number) {
    globalCounter += 1;
AssertType(globalCounter += 1, "number");
AssertType(globalCounter, "number");
AssertType(1, "int");

    let idx = Math.floor(Math.random() * max);
AssertType(idx, "number");
AssertType(Math.floor(Math.random() * max), "number");
AssertType(Math.floor, "(number) => number");
AssertType(Math.random() * max, "number");
AssertType(Math.random(), "number");
AssertType(Math.random, "() => number");
AssertType(max, "number");

AssertType(idx, "number");
    return idx;
}

let array1 = [1, 2, 3, 4, 5];
AssertType(array1, "number[]");
AssertType([1, 2, 3, 4, 5], "number[]");
AssertType(1, "int");
AssertType(2, "int");
AssertType(3, "int");
AssertType(4, "int");
AssertType(5, "int");

array1[incrementIdx(array1.length)] **= 3;
AssertType(array1[incrementIdx(array1.length)] **= 3, "number");
AssertType(array1[incrementIdx(array1.length)], "number");
AssertType(array1, "number[]");
AssertType(incrementIdx(array1.length), "number");
AssertType(incrementIdx, "(number) => number");
AssertType(array1.length, "number");
AssertType(3, "int");

array1[incrementIdx(array1.length)] **= array1[incrementIdx(array1.length)] **= 2;
AssertType(array1[incrementIdx(array1.length)] **= array1[incrementIdx(array1.length)] **= 2, "number");
AssertType(array1[incrementIdx(array1.length)], "number");
AssertType(array1, "number[]");
AssertType(incrementIdx(array1.length), "number");
AssertType(incrementIdx, "(number) => number");
AssertType(array1.length, "number");
AssertType(array1[incrementIdx(array1.length)] **= 2, "number");
AssertType(array1[incrementIdx(array1.length)], "number");
AssertType(array1, "number[]");
AssertType(incrementIdx(array1.length), "number");
AssertType(incrementIdx, "(number) => number");
AssertType(array1.length, "number");
AssertType(2, "int");

array1[incrementIdx(array1.length)] **= array1[incrementIdx(array1.length)] ** 2;
AssertType(array1[incrementIdx(array1.length)] **= array1[incrementIdx(array1.length)] ** 2, "number");
AssertType(array1[incrementIdx(array1.length)], "number");
AssertType(array1, "number[]");
AssertType(incrementIdx(array1.length), "number");
AssertType(incrementIdx, "(number) => number");
AssertType(array1.length, "number");
AssertType(array1[incrementIdx(array1.length)] ** 2, "number");
AssertType(array1[incrementIdx(array1.length)], "number");
AssertType(array1, "number[]");
AssertType(incrementIdx(array1.length), "number");
AssertType(incrementIdx, "(number) => number");
AssertType(array1.length, "number");
AssertType(2, "int");


