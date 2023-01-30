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

// === tests/cases/conformance/es6/Symbols/symbolProperty2.ts ===
declare function AssertType(value:any, type:string):void;
let s = Symbol();
AssertType(s, "symbol");
AssertType(Symbol(), "symbol");
AssertType(Symbol, "SymbolConstructor");

let x = {
AssertType(x, "{ [symbol]: union; }");
AssertType({    [s]: 0,    [s]() { },    get [s]() {        return 0;    }}, "{ [symbol]: union; }");

    [s]: 0,
AssertType([s], "number");
AssertType(s, "symbol");
AssertType(0, "int");

    [s]() { },
AssertType([s], "() => void");
AssertType(s, "symbol");

    get [s]() {
AssertType([s], "number");
AssertType(s, "symbol");

AssertType(0, "int");
        return 0;
    }
}

