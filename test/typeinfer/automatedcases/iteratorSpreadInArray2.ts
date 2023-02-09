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

// === tests/cases/conformance/es6/spread/iteratorSpreadInArray2.ts ===
declare function AssertType(value:any, type:string):void;
class SymbolIterator {
    next() {
AssertType({            value: Symbol(),            done: false        }, "{ value: symbol; done: boolean; }");
        return {

            value: Symbol(),
AssertType(value, "symbol");
AssertType(Symbol(), "symbol");
AssertType(Symbol, "SymbolConstructor");

            done: false
AssertType(done, "boolean");
AssertType(false, "boolean");

        };
    }

    [Symbol.iterator]() {
AssertType(this, "this");
        return this;
    }
}

class NumberIterator {
    next() {
AssertType({            value: 0,            done: false        }, "{ value: number; done: boolean; }");
        return {

            value: 0,
AssertType(value, "number");
AssertType(0, "int");

            done: false
AssertType(done, "boolean");
AssertType(false, "boolean");

        };
    }

    [Symbol.iterator]() {
AssertType(this, "this");
        return this;
    }
}

let array = [...new NumberIterator, ...new SymbolIterator];
AssertType(array, "(union)[]");
AssertType([...new NumberIterator, ...new SymbolIterator], "(union)[]");
AssertType(...new NumberIterator, "number");
AssertType(new NumberIterator, "NumberIterator");
AssertType(NumberIterator, "typeof NumberIterator");
AssertType(...new SymbolIterator, "symbol");
AssertType(new SymbolIterator, "SymbolIterator");
AssertType(SymbolIterator, "typeof SymbolIterator");


