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

// === tests/cases/conformance/jsdoc/declarations/index.js ===
declare function AssertType(value:any, type:string):void;
const TopLevelSym = Symbol();
AssertType(TopLevelSym, "unique symbol");
AssertType(Symbol(), "unique symbol");
AssertType(Symbol, "SymbolConstructor");

const InnerSym = Symbol();
AssertType(InnerSym, "unique symbol");
AssertType(Symbol(), "unique symbol");
AssertType(Symbol, "SymbolConstructor");

module.exports = {
AssertType(module.exports, "{ [TopLevelSym](?number): number; items: { [InnerSym]: (?{ x: number; }) => number; }; }");
AssertType({    [TopLevelSym](x = 12) {        return x;    },    items: {        [InnerSym]: (arg = {x: 12}) => arg.x    }}, "{ [TopLevelSym](?number): number; items: { [InnerSym]: (?{ x: number; }) => number; }; }");
AssertType(module.exports = {    [TopLevelSym](x = 12) {        return x;    },    items: {        [InnerSym]: (arg = {x: 12}) => arg.x    }}, "{ [TopLevelSym](?number): number; items: { [InnerSym]: (?{ x: number; }) => number; }; }");

    [TopLevelSym](x = 12) {
AssertType([TopLevelSym], "(?number) => number");
AssertType(TopLevelSym, "unique symbol");
AssertType(x, "number");
AssertType(12, "int");

AssertType(x, "number");
        return x;

    },
    items: {
AssertType(items, "{ [InnerSym]: (?{ x: number; }) => number; }");
AssertType({        [InnerSym]: (arg = {x: 12}) => arg.x    }, "{ [InnerSym]: (?{ x: number; }) => number; }");

        [InnerSym]: (arg = {x: 12}) => arg.x
AssertType([InnerSym], "(?{ x: number; }) => number");
AssertType(InnerSym, "unique symbol");
AssertType((arg = {x: 12}) => arg.x, "(?{ x: number; }) => number");
AssertType(arg, "{ x: number; }");
AssertType({x: 12}, "{ x: number; }");
AssertType(x, "number");
AssertType(12, "int");
AssertType(arg.x, "number");
    }
}


