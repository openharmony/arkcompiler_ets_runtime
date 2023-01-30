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

// === tests/cases/conformance/es6/for-ofStatements/for-of44.ts ===
declare function AssertType(value:any, type:string):void;
let array: [number, string | boolean | symbol][] = [[0, ""], [0, true], [1, Symbol()]]
AssertType(array, "[number, union][]");
AssertType([[0, ""], [0, true], [1, Symbol()]], "(union)[]");
AssertType([0, ""], "[number, string]");
AssertType(0, "int");
AssertType("", "string");
AssertType([0, true], "[number, true]");
AssertType(0, "int");
AssertType(true, "boolean");
AssertType([1, Symbol()], "[number, symbol]");
AssertType(1, "int");
AssertType(Symbol(), "symbol");
AssertType(Symbol, "SymbolConstructor");

for (let [num, strBoolSym] of array) {
    num;
AssertType(num, "number");

    strBoolSym;
AssertType(strBoolSym, "union");
}

