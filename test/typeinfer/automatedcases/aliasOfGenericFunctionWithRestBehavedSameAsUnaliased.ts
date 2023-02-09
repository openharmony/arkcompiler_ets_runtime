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

// === tests/cases/compiler/aliasOfGenericFunctionWithRestBehavedSameAsUnaliased.ts ===
declare function AssertType(value:any, type:string):void;
// the type printback for every `test` below should be "y"

type ExtendedMapper<HandledInputT, OutputT, ArgsT extends any[]> = (name : string, mixed : HandledInputT, ...args : ArgsT) => OutputT;
type a = ExtendedMapper<any, any, [any]>;
type b = ExtendedMapper<any, any, any[]>;
type test = a extends b ? "y" : "n"
let check: test = "y";
AssertType(check, "string");
AssertType("y", "string");


type ExtendedMapper1<HandledInputT, OutputT, ArgsT extends any[]> = (
    (name : string, mixed : HandledInputT, ...args : ArgsT) => OutputT
);

type a1 = ExtendedMapper1<any, any, [any]>;
type b1 = ExtendedMapper1<any, any, any[]>;
type test1 = a1 extends b1 ? "y" : "n"
let check1: test1 = "y";
AssertType(check1, "string");
AssertType("y", "string");

type ExtendedMapper2<HandledInputT, OutputT, ArgsT extends any[]> = (
    {x:(name : string, mixed : HandledInputT, ...args : ArgsT) => OutputT}["x"]
);

type a2 = ExtendedMapper2<any, any, [any]>;
type b2 = ExtendedMapper2<any, any, any[]>;
type test2 = a2 extends b2 ? "y" : "n"
let check2: test2 = "y";
AssertType(check2, "string");
AssertType("y", "string");

type a3 = (name: string, mixed: any, args_0: any) => any
type b3 = (name: string, mixed: any, ...args: any[]) => any

type test3 = a3 extends b3 ? "y" : "n"
let check3: test3 = "y";
AssertType(check3, "string");
AssertType("y", "string");


