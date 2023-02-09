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

// === tests/cases/conformance/types/rest/genericRestParameters2.ts ===
declare function AssertType(value:any, type:string):void;
declare const t1: [number, string, ...boolean[]];
AssertType(t1, "[number, string, ...boolean[]]");

declare const t2: [string, ...boolean[]];
AssertType(t2, "[string, ...boolean[]]");

declare const t3: [...boolean[]];
AssertType(t3, "boolean[]");

declare const t4: [];
AssertType(t4, "[]");

declare let f00: (...x: [number, string, boolean]) => void;
AssertType(f00, "(number, string, boolean) => void");
AssertType(x, "[number, string, boolean]");

declare let f01: (a: number, ...x: [string, boolean]) => void;
AssertType(f01, "(number, string, boolean) => void");
AssertType(a, "number");
AssertType(x, "[string, boolean]");

declare let f02: (a: number, b: string, ...x: [boolean]) => void;
AssertType(f02, "(number, string, boolean) => void");
AssertType(a, "number");
AssertType(b, "string");
AssertType(x, "[boolean]");

declare let f03: (a: number, b: string, c: boolean) => void;
AssertType(f03, "(number, string, boolean) => void");
AssertType(a, "number");
AssertType(b, "string");
AssertType(c, "boolean");

declare let f04: (a: number, b: string, c: boolean, ...x: []) => void;
AssertType(f04, "(number, string, boolean) => void");
AssertType(a, "number");
AssertType(b, "string");
AssertType(c, "boolean");
AssertType(x, "[]");

declare let f10: (...x: [number, string, ...boolean[]]) => void;
AssertType(f10, "(number, string, ...boolean[]) => void");
AssertType(x, "[number, string, ...boolean[]]");

declare let f11: (a: number, ...x: [string, ...boolean[]]) => void;
AssertType(f11, "(number, string, ...boolean[]) => void");
AssertType(a, "number");
AssertType(x, "[string, ...boolean[]]");

declare let f12: (a: number, b: string, ...x: [...boolean[]]) => void;
AssertType(f12, "(number, string, ...[...boolean[]]) => void");
AssertType(a, "number");
AssertType(b, "string");
AssertType(x, "boolean[]");

declare let f13: (a: number, b: string, ...c: boolean[]) => void;
AssertType(f13, "(number, string, ...boolean[]) => void");
AssertType(a, "number");
AssertType(b, "string");
AssertType(c, "boolean[]");

declare const ns: [number, string];
AssertType(ns, "[number, string]");

declare const sn: [string, number];
AssertType(sn, "[string, number]");

f10(42, "hello");
AssertType(f10(42, "hello"), "void");
AssertType(f10, "(number, string, ...boolean[]) => void");
AssertType(42, "int");
AssertType("hello", "string");

f10(42, "hello", true);
AssertType(f10(42, "hello", true), "void");
AssertType(f10, "(number, string, ...boolean[]) => void");
AssertType(42, "int");
AssertType("hello", "string");
AssertType(true, "boolean");

f10(42, "hello", true, false);
AssertType(f10(42, "hello", true, false), "void");
AssertType(f10, "(number, string, ...boolean[]) => void");
AssertType(42, "int");
AssertType("hello", "string");
AssertType(true, "boolean");
AssertType(false, "boolean");

f10(t1[0], t1[1], t1[2], t1[3]);
AssertType(f10(t1[0], t1[1], t1[2], t1[3]), "void");
AssertType(f10, "(number, string, ...boolean[]) => void");
AssertType(t1[0], "number");
AssertType(t1, "[number, string, ...boolean[]]");
AssertType(0, "int");
AssertType(t1[1], "string");
AssertType(t1, "[number, string, ...boolean[]]");
AssertType(1, "int");
AssertType(t1[2], "boolean");
AssertType(t1, "[number, string, ...boolean[]]");
AssertType(2, "int");
AssertType(t1[3], "boolean");
AssertType(t1, "[number, string, ...boolean[]]");
AssertType(3, "int");

f10(...t1);
AssertType(f10(...t1), "void");
AssertType(f10, "(number, string, ...boolean[]) => void");
AssertType(...t1, "union");
AssertType(t1, "[number, string, ...boolean[]]");

f10(42, ...t2);
AssertType(f10(42, ...t2), "void");
AssertType(f10, "(number, string, ...boolean[]) => void");
AssertType(42, "int");
AssertType(...t2, "union");
AssertType(t2, "[string, ...boolean[]]");

f10(42, "hello", ...t3);
AssertType(f10(42, "hello", ...t3), "void");
AssertType(f10, "(number, string, ...boolean[]) => void");
AssertType(42, "int");
AssertType("hello", "string");
AssertType(...t3, "boolean");
AssertType(t3, "boolean[]");

f10(42, "hello", true, ...t4);
AssertType(f10(42, "hello", true, ...t4), "void");
AssertType(f10, "(number, string, ...boolean[]) => void");
AssertType(42, "int");
AssertType("hello", "string");
AssertType(true, "boolean");
AssertType(...t4, "never");
AssertType(t4, "[]");

f10(42, "hello", true, ...t4, false, ...t3);
AssertType(f10(42, "hello", true, ...t4, false, ...t3), "void");
AssertType(f10, "(number, string, ...boolean[]) => void");
AssertType(42, "int");
AssertType("hello", "string");
AssertType(true, "boolean");
AssertType(...t4, "never");
AssertType(t4, "[]");
AssertType(false, "boolean");
AssertType(...t3, "boolean");
AssertType(t3, "boolean[]");

f11(42, "hello");
AssertType(f11(42, "hello"), "void");
AssertType(f11, "(number, string, ...boolean[]) => void");
AssertType(42, "int");
AssertType("hello", "string");

f11(42, "hello", true);
AssertType(f11(42, "hello", true), "void");
AssertType(f11, "(number, string, ...boolean[]) => void");
AssertType(42, "int");
AssertType("hello", "string");
AssertType(true, "boolean");

f11(42, "hello", true, false);
AssertType(f11(42, "hello", true, false), "void");
AssertType(f11, "(number, string, ...boolean[]) => void");
AssertType(42, "int");
AssertType("hello", "string");
AssertType(true, "boolean");
AssertType(false, "boolean");

f11(t1[0], t1[1], t1[2], t1[3]);
AssertType(f11(t1[0], t1[1], t1[2], t1[3]), "void");
AssertType(f11, "(number, string, ...boolean[]) => void");
AssertType(t1[0], "number");
AssertType(t1, "[number, string, ...boolean[]]");
AssertType(0, "int");
AssertType(t1[1], "string");
AssertType(t1, "[number, string, ...boolean[]]");
AssertType(1, "int");
AssertType(t1[2], "boolean");
AssertType(t1, "[number, string, ...boolean[]]");
AssertType(2, "int");
AssertType(t1[3], "boolean");
AssertType(t1, "[number, string, ...boolean[]]");
AssertType(3, "int");

f11(...t1);
AssertType(f11(...t1), "void");
AssertType(f11, "(number, string, ...boolean[]) => void");
AssertType(...t1, "union");
AssertType(t1, "[number, string, ...boolean[]]");

f11(42, ...t2);
AssertType(f11(42, ...t2), "void");
AssertType(f11, "(number, string, ...boolean[]) => void");
AssertType(42, "int");
AssertType(...t2, "union");
AssertType(t2, "[string, ...boolean[]]");

f11(42, "hello", ...t3);
AssertType(f11(42, "hello", ...t3), "void");
AssertType(f11, "(number, string, ...boolean[]) => void");
AssertType(42, "int");
AssertType("hello", "string");
AssertType(...t3, "boolean");
AssertType(t3, "boolean[]");

f11(42, "hello", true, ...t4);
AssertType(f11(42, "hello", true, ...t4), "void");
AssertType(f11, "(number, string, ...boolean[]) => void");
AssertType(42, "int");
AssertType("hello", "string");
AssertType(true, "boolean");
AssertType(...t4, "never");
AssertType(t4, "[]");

f11(42, "hello", true, ...t4, false, ...t3);
AssertType(f11(42, "hello", true, ...t4, false, ...t3), "void");
AssertType(f11, "(number, string, ...boolean[]) => void");
AssertType(42, "int");
AssertType("hello", "string");
AssertType(true, "boolean");
AssertType(...t4, "never");
AssertType(t4, "[]");
AssertType(false, "boolean");
AssertType(...t3, "boolean");
AssertType(t3, "boolean[]");

f12(42, "hello");
AssertType(f12(42, "hello"), "void");
AssertType(f12, "(number, string, ...boolean[]) => void");
AssertType(42, "int");
AssertType("hello", "string");

f12(42, "hello", true);
AssertType(f12(42, "hello", true), "void");
AssertType(f12, "(number, string, ...boolean[]) => void");
AssertType(42, "int");
AssertType("hello", "string");
AssertType(true, "boolean");

f12(42, "hello", true, false);
AssertType(f12(42, "hello", true, false), "void");
AssertType(f12, "(number, string, ...boolean[]) => void");
AssertType(42, "int");
AssertType("hello", "string");
AssertType(true, "boolean");
AssertType(false, "boolean");

f12(t1[0], t1[1], t1[2], t1[3]);
AssertType(f12(t1[0], t1[1], t1[2], t1[3]), "void");
AssertType(f12, "(number, string, ...boolean[]) => void");
AssertType(t1[0], "number");
AssertType(t1, "[number, string, ...boolean[]]");
AssertType(0, "int");
AssertType(t1[1], "string");
AssertType(t1, "[number, string, ...boolean[]]");
AssertType(1, "int");
AssertType(t1[2], "boolean");
AssertType(t1, "[number, string, ...boolean[]]");
AssertType(2, "int");
AssertType(t1[3], "boolean");
AssertType(t1, "[number, string, ...boolean[]]");
AssertType(3, "int");

f12(...t1);
AssertType(f12(...t1), "void");
AssertType(f12, "(number, string, ...boolean[]) => void");
AssertType(...t1, "union");
AssertType(t1, "[number, string, ...boolean[]]");

f12(42, ...t2);
AssertType(f12(42, ...t2), "void");
AssertType(f12, "(number, string, ...boolean[]) => void");
AssertType(42, "int");
AssertType(...t2, "union");
AssertType(t2, "[string, ...boolean[]]");

f12(42, "hello", ...t3);
AssertType(f12(42, "hello", ...t3), "void");
AssertType(f12, "(number, string, ...boolean[]) => void");
AssertType(42, "int");
AssertType("hello", "string");
AssertType(...t3, "boolean");
AssertType(t3, "boolean[]");

f12(42, "hello", true, ...t4);
AssertType(f12(42, "hello", true, ...t4), "void");
AssertType(f12, "(number, string, ...boolean[]) => void");
AssertType(42, "int");
AssertType("hello", "string");
AssertType(true, "boolean");
AssertType(...t4, "never");
AssertType(t4, "[]");

f12(42, "hello", true, ...t4, false, ...t3);
AssertType(f12(42, "hello", true, ...t4, false, ...t3), "void");
AssertType(f12, "(number, string, ...boolean[]) => void");
AssertType(42, "int");
AssertType("hello", "string");
AssertType(true, "boolean");
AssertType(...t4, "never");
AssertType(t4, "[]");
AssertType(false, "boolean");
AssertType(...t3, "boolean");
AssertType(t3, "boolean[]");

f13(42, "hello");
AssertType(f13(42, "hello"), "void");
AssertType(f13, "(number, string, ...boolean[]) => void");
AssertType(42, "int");
AssertType("hello", "string");

f13(42, "hello", true);
AssertType(f13(42, "hello", true), "void");
AssertType(f13, "(number, string, ...boolean[]) => void");
AssertType(42, "int");
AssertType("hello", "string");
AssertType(true, "boolean");

f13(42, "hello", true, false);
AssertType(f13(42, "hello", true, false), "void");
AssertType(f13, "(number, string, ...boolean[]) => void");
AssertType(42, "int");
AssertType("hello", "string");
AssertType(true, "boolean");
AssertType(false, "boolean");

f13(t1[0], t1[1], t1[2], t1[3]);
AssertType(f13(t1[0], t1[1], t1[2], t1[3]), "void");
AssertType(f13, "(number, string, ...boolean[]) => void");
AssertType(t1[0], "number");
AssertType(t1, "[number, string, ...boolean[]]");
AssertType(0, "int");
AssertType(t1[1], "string");
AssertType(t1, "[number, string, ...boolean[]]");
AssertType(1, "int");
AssertType(t1[2], "boolean");
AssertType(t1, "[number, string, ...boolean[]]");
AssertType(2, "int");
AssertType(t1[3], "boolean");
AssertType(t1, "[number, string, ...boolean[]]");
AssertType(3, "int");

f13(...t1);
AssertType(f13(...t1), "void");
AssertType(f13, "(number, string, ...boolean[]) => void");
AssertType(...t1, "union");
AssertType(t1, "[number, string, ...boolean[]]");

f13(42, ...t2);
AssertType(f13(42, ...t2), "void");
AssertType(f13, "(number, string, ...boolean[]) => void");
AssertType(42, "int");
AssertType(...t2, "union");
AssertType(t2, "[string, ...boolean[]]");

f13(42, "hello", ...t3);
AssertType(f13(42, "hello", ...t3), "void");
AssertType(f13, "(number, string, ...boolean[]) => void");
AssertType(42, "int");
AssertType("hello", "string");
AssertType(...t3, "boolean");
AssertType(t3, "boolean[]");

f13(42, "hello", true, ...t4);
AssertType(f13(42, "hello", true, ...t4), "void");
AssertType(f13, "(number, string, ...boolean[]) => void");
AssertType(42, "int");
AssertType("hello", "string");
AssertType(true, "boolean");
AssertType(...t4, "never");
AssertType(t4, "[]");

f13(42, "hello", true, ...t4, false, ...t3);
AssertType(f13(42, "hello", true, ...t4, false, ...t3), "void");
AssertType(f13, "(number, string, ...boolean[]) => void");
AssertType(42, "int");
AssertType("hello", "string");
AssertType(true, "boolean");
AssertType(...t4, "never");
AssertType(t4, "[]");
AssertType(false, "boolean");
AssertType(...t3, "boolean");
AssertType(t3, "boolean[]");

declare const f20: <T extends unknown[]>(...args: T) => T;
AssertType(f20, "<T extends unknown[]>(...T) => T");
AssertType(args, "T");

f20(...t1);
AssertType(f20(...t1), "[number, string, ...boolean[]]");
AssertType(f20, "<T extends unknown[]>(...T) => T");
AssertType(...t1, "union");
AssertType(t1, "[number, string, ...boolean[]]");

f20(42, ...t2);
AssertType(f20(42, ...t2), "[number, string, ...boolean[]]");
AssertType(f20, "<T extends unknown[]>(...T) => T");
AssertType(42, "int");
AssertType(...t2, "union");
AssertType(t2, "[string, ...boolean[]]");

f20(42, "hello", ...t3);
AssertType(f20(42, "hello", ...t3), "[number, string, ...boolean[]]");
AssertType(f20, "<T extends unknown[]>(...T) => T");
AssertType(42, "int");
AssertType("hello", "string");
AssertType(...t3, "boolean");
AssertType(t3, "boolean[]");

f20(42, "hello", ...t2, true);
AssertType(f20(42, "hello", ...t2, true), "[number, string, string, ...boolean[], boolean]");
AssertType(f20, "<T extends unknown[]>(...T) => T");
AssertType(42, "int");
AssertType("hello", "string");
AssertType(...t2, "union");
AssertType(t2, "[string, ...boolean[]]");
AssertType(true, "boolean");

type T01 = Parameters<(x: number, y: string, ...z: boolean[]) => void>;
type T02 = Parameters<(...args: [number, string, ...boolean[]]) => void>;
type T03 = ConstructorParameters<new (x: number, y: string, ...z: boolean[]) => void>;
type T04 = ConstructorParameters<new (...args: [number, string, ...boolean[]]) => void>;
type T05<T extends any[]> = Parameters<(x: string, ...args: T) => void>;
type T06 = T05<[number, ...boolean[]]>;

type P1<T extends Function> = T extends (head: infer A, ...tail: infer B) => any ? { head: A, tail: B } : any[];

type T10 = P1<(x: number, y: string, ...z: boolean[]) => void>;
type T11 = P1<(...z: number[]) => void>;
type T12 = P1<(x: number, y: number) => void>;


