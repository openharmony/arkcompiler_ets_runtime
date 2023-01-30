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

// === tests/cases/conformance/statements/for-ofStatements/ES5for-of32.ts ===
declare function AssertType(value:any, type:string):void;
let array = [1,2,3];
AssertType(array, "number[]");
AssertType([1,2,3], "number[]");
AssertType(1, "int");
AssertType(2, "int");
AssertType(3, "int");

let sum = 0;
AssertType(sum, "number");
AssertType(0, "int");

for (let num of array) {
    if (sum === 0) {
AssertType(sum === 0, "boolean");
AssertType(sum, "number");
AssertType(0, "int");

        array = [4,5,6]
AssertType(array = [4,5,6], "number[]");
AssertType(array, "number[]");
AssertType([4,5,6], "number[]");
AssertType(4, "int");
AssertType(5, "int");
AssertType(6, "int");
    }
    
    sum += num;
AssertType(sum += num, "number");
AssertType(sum, "number");
AssertType(num, "number");
}

