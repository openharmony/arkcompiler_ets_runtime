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

// === tests/cases/conformance/types/thisType/inferThisType.ts ===
declare function AssertType(value:any, type:string):void;
declare function f<T>(g: (this: T) => void): T
declare function h(this: number): void;
f(h)
AssertType(f(h), "number");
AssertType(f, "<T>((T) => void) => T");
AssertType(h, "(number) => void");

// works with infer types as well
type Check<T> = T extends (this: infer U, ...args: any[]) => any ? string : unknown;
type r1 = Check<(this: number) => void>; // should be string

type This<T>  = T extends (this: infer U, ...args: any[]) => any ? U : unknown;
type r2 = This<(this: number) => void>; // should be number


