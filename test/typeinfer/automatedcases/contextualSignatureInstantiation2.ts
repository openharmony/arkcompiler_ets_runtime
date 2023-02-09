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

// === tests/cases/compiler/contextualSignatureInstantiation2.ts ===
declare function AssertType(value:any, type:string):void;
// dot f g x = f(g(x))
let dot: <T, S>(f: (_: T) => S) => <U>(g: (_: U) => T) => (_: U) => S;
AssertType(dot, "<T, S>((T) => S) => <U>((U) => T) => (U) => S");
AssertType(f, "(T) => S");
AssertType(_, "T");
AssertType(g, "(U) => T");
AssertType(_, "U");
AssertType(_, "U");

dot = <T, S>(f: (_: T) => S) => <U>(g: (_: U) => T): (r:U) => S => (x) => f(g(x));
AssertType(dot = <T, S>(f: (_: T) => S) => <U>(g: (_: U) => T): (r:U) => S => (x) => f(g(x)), "<T, S>((T) => S) => <U>((U) => T) => (U) => S");
AssertType(dot, "<T, S>((T) => S) => <U>((U) => T) => (U) => S");
AssertType(<T, S>(f: (_: T) => S) => <U>(g: (_: U) => T): (r:U) => S => (x) => f(g(x)), "<T, S>((T) => S) => <U>((U) => T) => (U) => S");
AssertType(f, "(T) => S");
AssertType(_, "T");
AssertType(<U>(g: (_: U) => T): (r:U) => S => (x) => f(g(x)), "<U>((U) => T) => (U) => S");
AssertType(g, "(U) => T");
AssertType(_, "U");
AssertType(r, "U");
AssertType((x) => f(g(x)), "(U) => S");
AssertType(x, "U");
AssertType(f(g(x)), "S");
AssertType(f, "(T) => S");
AssertType(g(x), "T");
AssertType(g, "(U) => T");
AssertType(x, "U");

let id: <T>(x:T) => T;
AssertType(id, "<T>(T) => T");
AssertType(x, "T");

let r23 = dot(id)(id);
AssertType(r23, "<T>(T) => unknown");
AssertType(dot(id)(id), "<T>(T) => unknown");
AssertType(dot(id), "<U>((U) => unknown) => (U) => unknown");
AssertType(dot, "<T, S>((T) => S) => <U>((U) => T) => (U) => S");
AssertType(id, "<T>(T) => T");
AssertType(id, "<T>(T) => T");


