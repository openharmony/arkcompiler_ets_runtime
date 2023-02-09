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

// === tests/cases/conformance/es6/spread/iteratorSpreadInCall11.ts ===
declare function AssertType(value:any, type:string):void;
function foo<T>(...s: T[]) { 
AssertType(s[0], "T");
return s[0] 

AssertType(s, "T[]");

AssertType(0, "int");
}

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

foo(...new SymbolIterator);
AssertType(foo(...new SymbolIterator), "symbol");
AssertType(foo, "<T>(...T[]) => T");
AssertType(...new SymbolIterator, "symbol");
AssertType(new SymbolIterator, "SymbolIterator");
AssertType(SymbolIterator, "typeof SymbolIterator");


