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

// === tests/cases/conformance/functions/strictBindCallApply2.ts ===
declare function AssertType(value:any, type:string):void;
// Repro from #32964

interface Foo { blub: string };
function fn(this: Foo) {}

type Test = ThisParameterType<typeof fn>; 

const fb = fn.bind({ blub: "blub" });
AssertType(fb, "() => void");
AssertType(fn.bind({ blub: "blub" }), "() => void");
AssertType(fn.bind, "{ <T>(T, ThisParameterType<T>): OmitThisParameter<T>; <T, A0, A extends any[], R>((T, A0, ...A) => R, T, A0): (...A) => R; <T, A0, A1, A extends any[], R>((T, A0, A1, ...A) => R, T, A0, A1): (...A) => R; <T, A0, A1, A2, A extends any[], R>((T, A0, A1, A2, ...A) => R, T, A0, A1, A2): (...A) => R; <T, A0, A1, A2, A3, A extends any[], R>((T, A0, A1, A2, A3, ...A) => R, T, A0, A1, A2, A3): (...A) => R; <T, AX, R>((T, ...AX[]) => R, T, ...AX[]): (...AX[]) => R; }");
AssertType({ blub: "blub" }, "{ blub: string; }");
AssertType(blub, "string");
AssertType("blub", "string");


