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

// === tests/cases/compiler/argumentsAsPropertyName.ts ===
declare function AssertType(value:any, type:string):void;
// target: es5
type MyType = {
    arguments: Array<string>
}

declare function use(s: any);

function myFunction(myType: MyType) {
    for (let i = 0; i < 10; i++) {
AssertType(i, "number");
AssertType(0, "int");
AssertType(i < 10, "boolean");
AssertType(i, "number");
AssertType(10, "int");
AssertType(i++, "number");
AssertType(i, "number");

        use(myType.arguments[i]);
AssertType(use(myType.arguments[i]), "any");
AssertType(use, "(any) => any");
AssertType(myType.arguments[i], "string");
AssertType(myType.arguments, "string[]");
AssertType(i, "number");

        // create closure so that tsc will turn loop body into function
        const x = 5;
AssertType(x, "int");
AssertType(5, "int");

        [1, 2, 3].forEach(function(j) { use(x); })
AssertType([1, 2, 3].forEach(function(j) { use(x); }), "void");
AssertType([1, 2, 3].forEach, "((number, number, number[]) => void, ?any) => void");
AssertType([1, 2, 3], "number[]");
AssertType(1, "int");
AssertType(2, "int");
AssertType(3, "int");
AssertType(function(j) { use(x); }, "(number) => void");
AssertType(j, "number");
AssertType(use(x), "any");
AssertType(use, "(any) => any");
AssertType(x, "int");
    }
}

