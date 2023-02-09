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

// === tests/cases/compiler/typeAliasDeclarationEmit3.ts ===
declare function AssertType(value:any, type:string):void;
function f1(): void {
    for (let i = 0; i < 1; i++)
AssertType(i, "number");
AssertType(0, "int");
AssertType(i < 1, "boolean");
AssertType(i, "number");
AssertType(1, "int");
AssertType(i++, "number");
AssertType(i, "number");

        type foo = [];
AssertType(foo, "[]");

        console.log('f1');
AssertType(console.log('f1'), "void");
AssertType(console.log, "(...any[]) => void");
AssertType('f1', "string");
}

function f2(): void {
    while (true)
AssertType(true, "boolean");

        type foo = [];
AssertType(foo, "[]");

        console.log('f2');
AssertType(console.log('f2'), "void");
AssertType(console.log, "(...any[]) => void");
AssertType('f2', "string");
}

function f3(): void {
    if (true)
AssertType(true, "boolean");

        type foo = [];
AssertType(foo, "[]");

        console.log('f3');
AssertType(console.log('f3'), "void");
AssertType(console.log, "(...any[]) => void");
AssertType('f3', "string");
}


