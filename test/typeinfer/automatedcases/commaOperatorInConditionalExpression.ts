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

// === tests/cases/compiler/commaOperatorInConditionalExpression.ts ===
declare function AssertType(value:any, type:string):void;
function f (m: string) {
    [1, 2, 3].map(i => {
AssertType([1, 2, 3].map, "<U>((number, number, number[]) => U, ?any) => U[]");
AssertType([1, 2, 3], "number[]");
AssertType(1, "int");
AssertType(2, "int");
AssertType(3, "int");
AssertType(i => {        return true? { [m]: i } : { [m]: i + 1 }    }, "(number) => { [string]: number; }");
AssertType(i, "number");
AssertType([1, 2, 3].map(i => {        return true? { [m]: i } : { [m]: i + 1 }    }), "{ [string]: number; }[]");

AssertType(true? { [m]: i } : { [m]: i + 1 }, "{ [string]: number; }");
        return true? { [m]: i } : { [m]: i + 1 

AssertType(true, "boolean");

AssertType({ [m]: i }, "{ [string]: number; }");

AssertType([m], "number");

AssertType(m, "string");

AssertType(i, "number");

AssertType({ [m]: i + 1 }, "{ [string]: number; }");

AssertType([m], "number");

AssertType(m, "string");

AssertType(i + 1, "number");

AssertType(i, "number");

AssertType(1, "int");
}

    })
}

