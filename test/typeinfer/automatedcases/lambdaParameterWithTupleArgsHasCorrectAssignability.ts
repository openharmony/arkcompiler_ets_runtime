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

// === tests/cases/compiler/lambdaParameterWithTupleArgsHasCorrectAssignability.ts ===
declare function AssertType(value:any, type:string):void;
type MyTupleItem = {};
type MyTuple = [MyTupleItem, ...MyTupleItem[]];

type GenericFunction<T extends MyTuple> = (...fromArgs: T) => void;

class GenericClass<T extends MyTuple> {
    from: GenericFunction<T> | undefined;
}

function createClass<T extends MyTuple>(f: GenericFunction<T>): GenericClass<T> {
AssertType(new GenericClass<T>(/* ... use f */), "GenericClass<T>");
AssertType(GenericClass, "typeof GenericClass");
    return new GenericClass<T>(/* ... use f */);
}

function consumeClass(c: GenericClass<[string, boolean]>) { }

// should work
consumeClass(createClass(str => console.log(str.length)));
AssertType(consumeClass(createClass(str => console.log(str.length))), "void");
AssertType(consumeClass, "(GenericClass<[string, boolean]>) => void");
AssertType(createClass(str => console.log(str.length)), "GenericClass<[str: string]>");
AssertType(createClass, "<T extends MyTuple>(GenericFunction<T>) => GenericClass<T>");
AssertType(str => console.log(str.length), "(string) => void");
AssertType(str, "string");
AssertType(console.log(str.length), "void");
AssertType(console.log, "(...any[]) => void");
AssertType(str.length, "number");

// should work
consumeClass(createClass((str, _unused_num) => console.log(str.length)));
AssertType(consumeClass(createClass((str, _unused_num) => console.log(str.length))), "void");
AssertType(consumeClass, "(GenericClass<[string, boolean]>) => void");
AssertType(createClass((str, _unused_num) => console.log(str.length)), "GenericClass<[str: string, _unused_num: boolean]>");
AssertType(createClass, "<T extends MyTuple>(GenericFunction<T>) => GenericClass<T>");
AssertType((str, _unused_num) => console.log(str.length), "(string, boolean) => void");
AssertType(str, "string");
AssertType(_unused_num, "boolean");
AssertType(console.log(str.length), "void");
AssertType(console.log, "(...any[]) => void");
AssertType(str.length, "number");


