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

// === tests/cases/compiler/argumentsObjectIterator02_ES6.ts ===
declare function AssertType(value:any, type:string):void;
function doubleAndReturnAsArray(x: number, y: number, z: number): [number, number, number] {
    let blah = arguments[Symbol.iterator];
AssertType(blah, "() => IterableIterator<any>");
AssertType(arguments[Symbol.iterator], "() => IterableIterator<any>");
AssertType(arguments, "IArguments");
AssertType(Symbol.iterator, "unique symbol");

    let result = [];
AssertType(result, "any[]");
AssertType([], "undefined[]");

    for (let arg of blah()) {
AssertType(arg, "any");
AssertType(blah(), "IterableIterator<any>");
AssertType(blah, "() => IterableIterator<any>");

        result.push(arg + arg);
AssertType(result.push(arg + arg), "number");
AssertType(result.push, "(...any[]) => number");
AssertType(arg + arg, "any");
AssertType(arg, "any");
AssertType(arg, "any");
    }
AssertType(<[any, any, any]>result, "[any, any, any]");
AssertType(result, "any[]");
    return <[any, any, any]>result;
}



