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

// === tests/cases/conformance/es6/destructuring/destructuringObjectBindingPatternAndAssignment7.ts ===
declare function AssertType(value:any, type:string):void;
enum K {
    a = "a",
    b = "b"
}
const { [K.a]: aVal, [K.b]: bVal } = (() => {
AssertType(K.a, "K.a");
AssertType(aVal, "number");
AssertType(K.b, "K.b");
AssertType(bVal, "number");
AssertType((() => {	return { [K.a]: 1, [K.b]: 1 };}), "() => { a: number; b: number; }");
AssertType(() => {	return { [K.a]: 1, [K.b]: 1 };}, "() => { a: number; b: number; }");
AssertType((() => {	return { [K.a]: 1, [K.b]: 1 };})(), "{ a: number; b: number; }");

AssertType({ [K.a]: 1, [K.b]: 1 }, "{ a: number; b: number; }");
AssertType([K.a], "number");
AssertType(K.a, "K.a");
AssertType(1, "int");
AssertType([K.b], "number");
AssertType(K.b, "K.b");
AssertType(1, "int");
	return { [K.a]: 1, [K.b]: 1 };

})();
console.log(aVal, bVal);
AssertType(console.log(aVal, bVal), "void");
AssertType(console.log, "(...any[]) => void");
AssertType(aVal, "number");
AssertType(bVal, "number");


