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

// === tests/cases/compiler/genericFunctionsNotContextSensitive.ts ===
declare function AssertType(value:any, type:string):void;
// Repro from #37110

const f = <F extends (...args: any[]) => <G>(x: G) => void>(_: F): F => _;
AssertType(f, "<F extends (...any[]) => <G>(G) => void>(F) => F");
AssertType(<F extends (...args: any[]) => <G>(x: G) => void>(_: F): F => _, "<F extends (...any[]) => <G>(G) => void>(F) => F");
AssertType(args, "any[]");
AssertType(x, "G");
AssertType(_, "F");
AssertType(_, "F");

const a = f(<K extends string>(_: K) => _ => ({}));  // <K extends string>(_: K) => <G>(_: G) => {
AssertType(a, "<K extends string>(K) => <G>(G) => {}");

AssertType(f(<K extends string>(_: K) => _ => ({})), "<K extends string>(K) => <G>(G) => {}");

AssertType(f, "<F extends (...any[]) => <G>(G) => void>(F) => F");

AssertType(<K extends string>(_: K) => _ => ({}), "<K extends string>(K) => <G>(G) => {}");

AssertType(_, "K");

AssertType(_ => ({}), "<G>(G) => {}");

AssertType(_, "G");

AssertType(({}), "{}");

AssertType({}, "{}");
}


