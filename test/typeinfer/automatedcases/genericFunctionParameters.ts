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

// === tests/cases/conformance/types/typeRelationships/typeInference/genericFunctionParameters.ts ===
declare function AssertType(value:any, type:string):void;
declare function f1<T>(cb: <S>(x: S) => T): T;
declare function f2<T>(cb: <S extends number>(x: S) => T): T;
declare function f3<T>(cb: <S extends Array<S>>(x: S) => T): T;

let x1 = f1(x => x);  // {
AssertType(x1, "unknown");

AssertType(f1(x => x), "unknown");

AssertType(f1, "<T>(<S>(S) => T) => T");

AssertType(x => x, "<S>(S) => S");

AssertType(x, "S");

AssertType(x, "S");
}

let x2 = f2(x => x);  // number
AssertType(x2, "number");
AssertType(f2(x => x), "number");
AssertType(f2, "<T>(<S extends number>(S) => T) => T");
AssertType(x => x, "<S extends number>(S) => S");
AssertType(x, "S");
AssertType(x, "S");

let x3 = f3(x => x);  // Array<any>
AssertType(x3, "any[]");
AssertType(f3(x => x), "any[]");
AssertType(f3, "<T>(<S extends S[]>(S) => T) => T");
AssertType(x => x, "<S extends S[]>(S) => S");
AssertType(x, "S");
AssertType(x, "S");

// Repro from #19345

declare const s: <R>(go: <S>(ops: { init(): S; }) => R) => R;
AssertType(s, "<R>(<S>({ init(): S; }) => R) => R");
AssertType(go, "<S>({ init(): S; }) => R");
AssertType(ops, "{ init(): S; }");
AssertType(init, "() => S");

const x = s(a => a.init());  // x is any, should have been {
AssertType(x, "unknown");

AssertType(s(a => a.init()), "unknown");

AssertType(s, "<R>(<S>({ init(): S; }) => R) => R");

AssertType(a => a.init(), "<S>({ init(): S; }) => S");

AssertType(a, "{ init(): S; }");

AssertType(a.init(), "S");

AssertType(a.init, "() => S");
}


