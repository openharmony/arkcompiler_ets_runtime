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

// === tests/cases/conformance/es6/destructuring/iterableArrayPattern20.ts ===
declare function AssertType(value:any, type:string):void;
class Bar { x }
class Foo extends Bar { y }
class FooArrayIterator {
    next() {
AssertType({            value: [new Foo],            done: false        }, "{ value: Foo[]; done: boolean; }");
        return {

            value: [new Foo],
AssertType(value, "Foo[]");
AssertType([new Foo], "Foo[]");
AssertType(new Foo, "Foo");
AssertType(Foo, "typeof Foo");

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

function fun(...[[a = new Foo], b = [new Foo]]: Bar[][]) { }
fun(...new FooArrayIterator);
AssertType(fun(...new FooArrayIterator), "void");
AssertType(fun, "(...Bar[][]) => void");
AssertType(...new FooArrayIterator, "Foo[]");
AssertType(new FooArrayIterator, "FooArrayIterator");
AssertType(FooArrayIterator, "typeof FooArrayIterator");


