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

// === tests/cases/compiler/recursiveConditionalEvaluationNonInfinite.ts ===
declare function AssertType(value:any, type:string):void;
type Test<T> = [T] extends [any[]] ? { array: Test<T[0]> } : { notArray: T };
declare const x: Test<number[]>;
AssertType(x, "{ array: { notArray: number; }; }");

const y: { array: { notArray: number } } = x; // Error
AssertType(y, "{ array: {    notArray: number;}; }");
AssertType(array, "{ notArray: number; }");
AssertType(notArray, "number");
AssertType(x, "{ array: { notArray: number; }; }");

declare const a: Test<number>;
AssertType(a, "{ notArray: number; }");

const b: { notArray: number } = a; // Works
AssertType(b, "{ notArray: number; }");
AssertType(notArray, "number");
AssertType(a, "{ notArray: number; }");


