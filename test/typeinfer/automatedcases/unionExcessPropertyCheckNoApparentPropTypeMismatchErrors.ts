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

// === tests/cases/compiler/unionExcessPropertyCheckNoApparentPropTypeMismatchErrors.ts ===
declare function AssertType(value:any, type:string):void;
interface IStringDictionary<V> {
	[name: string]: V;
}
interface INumberDictionary<V> {
	[idx: number]: V;
}

declare function forEach<T>(from: IStringDictionary<T> | INumberDictionary<T>, callback: (entry: { key: any; value: T; }, remove: () => void) => any);

let count = 0;
AssertType(count, "number");
AssertType(0, "int");

forEach({ toString: 123 }, () => count++);
AssertType(forEach({ toString: 123 }, () => count++), "any");
AssertType(forEach, "<T>(union, ({ key: any; value: T; }, () => void) => any) => any");
AssertType({ toString: 123 }, "{ toString: number; }");
AssertType(toString, "number");
AssertType(123, "int");
AssertType(() => count++, "() => number");
AssertType(count++, "number");
AssertType(count, "number");


