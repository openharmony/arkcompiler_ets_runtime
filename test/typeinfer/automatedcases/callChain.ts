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

// === tests/cases/conformance/expressions/optionalChaining/callChain/callChain.ts ===
declare function AssertType(value:any, type:string):void;
declare const o1: undefined | ((...args: any[]) => number);
AssertType(o1, "union");
AssertType(args, "any[]");

o1?.();
AssertType(o1?.(), "union");
AssertType(o1, "union");

o1?.(1);
AssertType(o1?.(1), "union");
AssertType(o1, "union");
AssertType(1, "int");

o1?.(...[1, 2]);
AssertType(o1?.(...[1, 2]), "union");
AssertType(o1, "union");
AssertType(...[1, 2], "number");
AssertType([1, 2], "number[]");
AssertType(1, "int");
AssertType(2, "int");

o1?.(1, ...[2, 3], 4);
AssertType(o1?.(1, ...[2, 3], 4), "union");
AssertType(o1, "union");
AssertType(1, "int");
AssertType(...[2, 3], "number");
AssertType([2, 3], "number[]");
AssertType(2, "int");
AssertType(3, "int");
AssertType(4, "int");

declare const o2: undefined | { b: (...args: any[]) => number };
AssertType(o2, "union");
AssertType(b, "(...any[]) => number");
AssertType(args, "any[]");

o2?.b();
AssertType(o2?.b(), "union");
AssertType(o2?.b, "union");

o2?.b(1);
AssertType(o2?.b(1), "union");
AssertType(o2?.b, "union");
AssertType(1, "int");

o2?.b(...[1, 2]);
AssertType(o2?.b(...[1, 2]), "union");
AssertType(o2?.b, "union");
AssertType(...[1, 2], "number");
AssertType([1, 2], "number[]");
AssertType(1, "int");
AssertType(2, "int");

o2?.b(1, ...[2, 3], 4);
AssertType(o2?.b(1, ...[2, 3], 4), "union");
AssertType(o2?.b, "union");
AssertType(1, "int");
AssertType(...[2, 3], "number");
AssertType([2, 3], "number[]");
AssertType(2, "int");
AssertType(3, "int");
AssertType(4, "int");

o2?.["b"]();
AssertType(o2?.["b"](), "union");
AssertType(o2?.["b"], "union");
AssertType(o2, "union");
AssertType("b", "string");

o2?.["b"](1);
AssertType(o2?.["b"](1), "union");
AssertType(o2?.["b"], "union");
AssertType(o2, "union");
AssertType("b", "string");
AssertType(1, "int");

o2?.["b"](...[1, 2]);
AssertType(o2?.["b"](...[1, 2]), "union");
AssertType(o2?.["b"], "union");
AssertType(o2, "union");
AssertType("b", "string");
AssertType(...[1, 2], "number");
AssertType([1, 2], "number[]");
AssertType(1, "int");
AssertType(2, "int");

o2?.["b"](1, ...[2, 3], 4);
AssertType(o2?.["b"](1, ...[2, 3], 4), "union");
AssertType(o2?.["b"], "union");
AssertType(o2, "union");
AssertType("b", "string");
AssertType(1, "int");
AssertType(...[2, 3], "number");
AssertType([2, 3], "number[]");
AssertType(2, "int");
AssertType(3, "int");
AssertType(4, "int");

declare const o3: { b: ((...args: any[]) => { c: string }) | undefined };
AssertType(o3, "{ b: union; }");
AssertType(b, "union");
AssertType(args, "any[]");
AssertType(c, "string");

o3.b?.().c;
AssertType(o3.b?.().c, "union");

o3.b?.(1).c;
AssertType(o3.b?.(1).c, "union");

o3.b?.(...[1, 2]).c;
AssertType(o3.b?.(...[1, 2]).c, "union");

o3.b?.(1, ...[2, 3], 4).c;
AssertType(o3.b?.(1, ...[2, 3], 4).c, "union");

o3.b?.()["c"];
AssertType(o3.b?.()["c"], "union");
AssertType(o3.b?.(), "union");
AssertType(o3.b, "union");
AssertType("c", "string");

o3.b?.(1)["c"];
AssertType(o3.b?.(1)["c"], "union");
AssertType(o3.b?.(1), "union");
AssertType(o3.b, "union");
AssertType(1, "int");
AssertType("c", "string");

o3.b?.(...[1, 2])["c"];
AssertType(o3.b?.(...[1, 2])["c"], "union");
AssertType(o3.b?.(...[1, 2]), "union");
AssertType(o3.b, "union");
AssertType(...[1, 2], "number");
AssertType([1, 2], "number[]");
AssertType(1, "int");
AssertType(2, "int");
AssertType("c", "string");

o3.b?.(1, ...[2, 3], 4)["c"];
AssertType(o3.b?.(1, ...[2, 3], 4)["c"], "union");
AssertType(o3.b?.(1, ...[2, 3], 4), "union");
AssertType(o3.b, "union");
AssertType(1, "int");
AssertType(...[2, 3], "number");
AssertType([2, 3], "number[]");
AssertType(2, "int");
AssertType(3, "int");
AssertType(4, "int");
AssertType("c", "string");

o3["b"]?.().c;
AssertType(o3["b"]?.().c, "union");

o3["b"]?.(1).c;
AssertType(o3["b"]?.(1).c, "union");

o3["b"]?.(...[1, 2]).c;
AssertType(o3["b"]?.(...[1, 2]).c, "union");

o3["b"]?.(1, ...[2, 3], 4).c;
AssertType(o3["b"]?.(1, ...[2, 3], 4).c, "union");

declare const o4: undefined | (<T>(f: (a: T) => T) => T);
AssertType(o4, "union");
AssertType(f, "(T) => T");
AssertType(a, "T");

declare function incr(x: number): number;
const v: number | undefined = o4?.(incr);
AssertType(v, "union");
AssertType(o4?.(incr), "union");
AssertType(o4, "union");
AssertType(incr, "(number) => number");

// GH#33744
declare const o5: <T>() => undefined | (() => void);
AssertType(o5, "<T>() => union");

o5<number>()?.();
AssertType(o5<number>()?.(), "union");
AssertType(o5<number>(), "union");
AssertType(o5, "<T>() => union");

// GH#36031
o2?.b()!.toString;
AssertType(o2?.b()!.toString, "union");

o2?.b()!.toString!;
AssertType(o2?.b()!.toString!, "(?union) => string");
AssertType(o2?.b()!.toString, "union");


