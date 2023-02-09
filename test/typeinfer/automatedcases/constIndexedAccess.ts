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

// === tests/cases/compiler/constIndexedAccess.ts ===
declare function AssertType(value:any, type:string):void;
const enum numbers {
    zero,
    one
}

interface indexAccess {
    0: string;
    1: number;
}

let test: indexAccess;
AssertType(test, "indexAccess");

let s = test[0];
AssertType(s, "string");
AssertType(test[0], "string");
AssertType(test, "indexAccess");
AssertType(0, "int");

let n = test[1];
AssertType(n, "number");
AssertType(test[1], "number");
AssertType(test, "indexAccess");
AssertType(1, "int");

let s1 = test[numbers.zero];
AssertType(s1, "string");
AssertType(test[numbers.zero], "string");
AssertType(test, "indexAccess");
AssertType(numbers.zero, "numbers.zero");

let n1 = test[numbers.one];
AssertType(n1, "number");
AssertType(test[numbers.one], "number");
AssertType(test, "indexAccess");
AssertType(numbers.one, "numbers.one");

let s2 = test[numbers["zero"]];
AssertType(s2, "string");
AssertType(test[numbers["zero"]], "string");
AssertType(test, "indexAccess");
AssertType(numbers["zero"], "numbers.zero");
AssertType(numbers, "typeof numbers");
AssertType("zero", "string");

let n2 = test[numbers["one"]];
AssertType(n2, "number");
AssertType(test[numbers["one"]], "number");
AssertType(test, "indexAccess");
AssertType(numbers["one"], "numbers.one");
AssertType(numbers, "typeof numbers");
AssertType("one", "string");

enum numbersNotConst {
    zero,
    one
}

let s3 = test[numbersNotConst.zero];
AssertType(s3, "string");
AssertType(test[numbersNotConst.zero], "string");
AssertType(test, "indexAccess");
AssertType(numbersNotConst.zero, "numbersNotConst.zero");

let n3 = test[numbersNotConst.one];
AssertType(n3, "number");
AssertType(test[numbersNotConst.one], "number");
AssertType(test, "indexAccess");
AssertType(numbersNotConst.one, "numbersNotConst.one");


