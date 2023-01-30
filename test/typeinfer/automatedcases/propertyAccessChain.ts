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

// === tests/cases/conformance/expressions/optionalChaining/propertyAccessChain/propertyAccessChain.ts ===
declare function AssertType(value:any, type:string):void;
declare const o1: undefined | { b: string };
AssertType(o1, "union");
AssertType(b, "string");

o1?.b;
AssertType(o1?.b, "union");

declare const o2: undefined | { b: { c: string } };
AssertType(o2, "union");
AssertType(b, "{ c: string; }");
AssertType(c, "string");

o2?.b.c;
AssertType(o2?.b.c, "union");

declare const o3: { b: undefined | { c: string } };
AssertType(o3, "{ b: union; }");
AssertType(b, "union");
AssertType(c, "string");

o3.b?.c;
AssertType(o3.b?.c, "union");

declare const o4: { b?: { c: { d?: { e: string } } } };
AssertType(o4, "{ b?: union; }");
AssertType(b, "union");
AssertType(c, "{ d?: union; }");
AssertType(d, "union");
AssertType(e, "string");

o4.b?.c.d?.e;
AssertType(o4.b?.c.d?.e, "union");

declare const o5: { b?(): { c: { d?: { e: string } } } };
AssertType(o5, "{ b?(): {    c: {        d?: {            e: string;        };    };}; }");
AssertType(b, "union");
AssertType(c, "{ d?: union; }");
AssertType(d, "union");
AssertType(e, "string");

o5.b?.().c.d?.e;
AssertType(o5.b?.().c.d?.e, "union");

// GH#33744
declare const o6: <T>() => undefined | ({ x: number });
AssertType(o6, "<T>() => union");
AssertType(x, "number");

o6<number>()?.x;
AssertType(o6<number>()?.x, "union");

// GH#34109
o1?.b ? 1 : 0;
AssertType(o1?.b ? 1 : 0, "union");
AssertType(o1?.b, "union");
AssertType(1, "int");
AssertType(0, "int");

// GH#36031
o2?.b!.c;
AssertType(o2?.b!.c, "union");

o2?.b!.c!;
AssertType(o2?.b!.c!, "string");
AssertType(o2?.b!.c, "union");


