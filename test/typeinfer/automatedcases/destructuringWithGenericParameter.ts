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

// === tests/cases/compiler/destructuringWithGenericParameter.ts ===
declare function AssertType(value:any, type:string):void;
class GenericClass<T> {
    payload: T;
}

let genericObject = new GenericClass<{ greeting: string }>();
AssertType(genericObject, "GenericClass<{ greeting: string; }>");
AssertType(new GenericClass<{ greeting: string }>(), "GenericClass<{ greeting: string; }>");
AssertType(GenericClass, "typeof GenericClass");
AssertType(greeting, "string");

function genericFunction<T>(object: GenericClass<T>, callback: (payload: T) => void) {
    callback(object.payload);
AssertType(callback(object.payload), "void");
AssertType(callback, "(T) => void");
AssertType(object.payload, "T");
}

genericFunction(genericObject, ({greeting}) => {
AssertType(genericFunction(genericObject, ({greeting}) => {    let s = greeting.toLocaleLowerCase();  // Greeting should be of type string}), "void");
AssertType(genericFunction, "<T>(GenericClass<T>, (T) => void) => void");
AssertType(genericObject, "GenericClass<{ greeting: string; }>");
AssertType(({greeting}) => {    let s = greeting.toLocaleLowerCase();  // Greeting should be of type string}, "({ greeting: string; }) => void");
AssertType(greeting, "string");

    let s = greeting.toLocaleLowerCase();  // Greeting should be of type string
AssertType(s, "string");
AssertType(greeting.toLocaleLowerCase(), "string");
AssertType(greeting.toLocaleLowerCase, "(?union) => string");

});


