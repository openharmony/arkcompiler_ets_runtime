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

// === tests/cases/conformance/types/contextualTypes/partiallyAnnotatedFunction/partiallyAnnotatedFunctionWitoutTypeParameter.ts ===
declare function AssertType(value:any, type:string):void;
// simple case
declare function simple(f: (a: number, b: number) => void): {}

simple((a: number, b) => {})
AssertType(simple((a: number, b) => {}), "{}");
AssertType(simple, "((number, number) => void) => {}");
AssertType((a: number, b) => {}, "(number, number) => void");
AssertType(a, "number");
AssertType(b, "number");

simple((a, b: number) => {})
AssertType(simple((a, b: number) => {}), "{}");
AssertType(simple, "((number, number) => void) => {}");
AssertType((a, b: number) => {}, "(number, number) => void");
AssertType(a, "number");
AssertType(b, "number");


