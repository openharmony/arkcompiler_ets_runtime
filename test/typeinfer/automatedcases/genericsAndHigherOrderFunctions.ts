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

// === tests/cases/compiler/genericsAndHigherOrderFunctions.ts ===
declare function AssertType(value:any, type:string):void;
// no errors expected

let combine: <T, S>(f: (_: T) => S) =>
AssertType(combine, "<T, S>((T) => S) => <U>((U) => T) => (U) => S");
AssertType(f, "(T) => S");
AssertType(_, "T");

    <U>(g: (_: U) => T) =>
AssertType(g, "(U) => T");
AssertType(_, "U");

    (x: U) => S
AssertType(x, "U");

    = <T, S>(f: (_: T) => S) =>
AssertType(<T, S>(f: (_: T) => S) =>        <U>(g: (_: U) => T) =>            (x: U) => f(g(x)), "<T, S>((T) => S) => <U>((U) => T) => (U) => S");
AssertType(f, "(T) => S");
AssertType(_, "T");

        <U>(g: (_: U) => T) =>
AssertType(<U>(g: (_: U) => T) =>            (x: U) => f(g(x)), "<U>((U) => T) => (U) => S");
AssertType(g, "(U) => T");
AssertType(_, "U");

            (x: U) => f(g(x))
AssertType((x: U) => f(g(x)), "(U) => S");
AssertType(x, "U");
AssertType(f(g(x)), "S");
AssertType(f, "(T) => S");
AssertType(g(x), "T");
AssertType(g, "(U) => T");
AssertType(x, "U");

let foo: <K, N>(g: (x: K) => N) =>
AssertType(foo, "<K, N>((K) => N) => (<M>((K) => (M) => M) => (M) => M) => <R>((N) => (R) => R) => (R) => R");
AssertType(g, "(K) => N");
AssertType(x, "K");

    (h: <M>(_: (_: K) => (_: M) => M) => (_: M) => M) =>
AssertType(h, "<M>((K) => (M) => M) => (M) => M");
AssertType(_, "(K) => (M) => M");
AssertType(_, "K");
AssertType(_, "M");
AssertType(_, "M");

    <R>(f: (_: N) => (_: R) => R) => (_: R) => R
AssertType(f, "(N) => (R) => R");
AssertType(_, "N");
AssertType(_, "R");
AssertType(_, "R");

    = <K, N>(g: (x: K) => N) =>
AssertType(<K, N>(g: (x: K) => N) =>        (h: <M>(_: (_: K) => (_: M) => M) => (_: M) => M) =>            <R>(f: (_: N) => (_: R) => R) => h(combine(f)(g)), "<K, N>((K) => N) => (<M>((K) => (M) => M) => (M) => M) => <R>((N) => (R) => R) => (R) => R");
AssertType(g, "(K) => N");
AssertType(x, "K");

        (h: <M>(_: (_: K) => (_: M) => M) => (_: M) => M) =>
AssertType((h: <M>(_: (_: K) => (_: M) => M) => (_: M) => M) =>            <R>(f: (_: N) => (_: R) => R) => h(combine(f)(g)), "(<M>((K) => (M) => M) => (M) => M) => <R>((N) => (R) => R) => (R) => R");
AssertType(h, "<M>((K) => (M) => M) => (M) => M");
AssertType(_, "(K) => (M) => M");
AssertType(_, "K");
AssertType(_, "M");
AssertType(_, "M");

            <R>(f: (_: N) => (_: R) => R) => h(combine(f)(g))
AssertType(<R>(f: (_: N) => (_: R) => R) => h(combine(f)(g)), "<R>((N) => (R) => R) => (R) => R");
AssertType(f, "(N) => (R) => R");
AssertType(_, "N");
AssertType(_, "R");
AssertType(h(combine(f)(g)), "(R) => R");
AssertType(h, "<M>((K) => (M) => M) => (M) => M");
AssertType(combine(f)(g), "(K) => (R) => R");
AssertType(combine(f), "<U>((U) => N) => (U) => (R) => R");
AssertType(combine, "<T, S>((T) => S) => <U>((U) => T) => (U) => S");
AssertType(f, "(N) => (R) => R");
AssertType(g, "(K) => N");


