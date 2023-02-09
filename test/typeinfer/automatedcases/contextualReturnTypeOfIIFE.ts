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

// === tests/cases/compiler/contextualReturnTypeOfIIFE.ts ===
declare function AssertType(value:any, type:string):void;
const test1: Promise<[one: number, two: string]> = (async () => {
AssertType(test1, "Promise<[one: number, two: string]>");
AssertType((async () => {    return [1, 'two'];}), "() => Promise<[number, string]>");
AssertType(async () => {    return [1, 'two'];}, "() => Promise<[number, string]>");
AssertType((async () => {    return [1, 'two'];})(), "Promise<[number, string]>");

AssertType([1, 'two'], "[number, string]");
AssertType(1, "int");
AssertType('two', "string");
    return [1, 'two'];

})();

const test2: Promise<[one: number, two: string]> = new Promise(
AssertType(test2, "Promise<[one: number, two: string]>");
AssertType(new Promise(    (resolve) => resolve([1, 'two']),), "Promise<[one: number, two: string]>");
AssertType(Promise, "PromiseConstructor");

    (resolve) => resolve([1, 'two']),
AssertType((resolve) => resolve([1, 'two']), "((union) => void) => void");
AssertType(resolve, "(union) => void");
AssertType(resolve([1, 'two']), "void");
AssertType(resolve, "(union) => void");
AssertType([1, 'two'], "[number, string]");
AssertType(1, "int");
AssertType('two', "string");

);

const obj: { foo: [one: number, two: string] } = {
AssertType(obj, "{ foo: [one: number, two: string]; }");
AssertType(foo, "[one: number, two: string]");
AssertType({    foo: (() => [1, 'two'])()}, "{ foo: [number, string]; }");

    foo: (() => [1, 'two'])()
AssertType(foo, "[number, string]");
AssertType((() => [1, 'two'])(), "[number, string]");
AssertType((() => [1, 'two']), "() => [number, string]");
AssertType(() => [1, 'two'], "() => [number, string]");
AssertType([1, 'two'], "[number, string]");
AssertType(1, "int");
AssertType('two', "string");

};


