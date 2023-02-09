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

// === tests/cases/compiler/arrayLiteralComments.ts ===
declare function AssertType(value:any, type:string):void;
let testArrayWithFunc = [
AssertType(testArrayWithFunc, "(union)[]");
AssertType([    // Function comment    function() {        let x = 1;    },    // String comment    '1',    // Numeric comment    2,    // Object comment    { a: 1 },    // Array comment    [1, 2, 3]], "(union)[]");

    // Function comment
    function() {
AssertType(function() {        let x = 1;    }, "() => void");

        let x = 1;
AssertType(x, "number");
AssertType(1, "int");

    },
    // String comment
    '1',
AssertType('1', "string");

    // Numeric comment
    2,
AssertType(2, "int");

    // Object comment
    { a: 1 },
AssertType({ a: 1 }, "{ a: number; }");
AssertType(a, "number");
AssertType(1, "int");

    // Array comment
    [1, 2, 3]
AssertType([1, 2, 3], "number[]");
AssertType(1, "int");
AssertType(2, "int");
AssertType(3, "int");

]

