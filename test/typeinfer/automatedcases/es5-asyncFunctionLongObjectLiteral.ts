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

// === tests/cases/compiler/es5-asyncFunctionLongObjectLiteral.ts ===
declare function AssertType(value:any, type:string):void;
// the generated code from both should be similar

const fooShort = async () => {
AssertType(fooShort, "() => Promise<{ a: number; b: number; c: number; d: number; e: number; }>");
AssertType(async () => {    return {        a: await Promise.resolve(0),        b: await Promise.resolve(1),        c: await Promise.resolve(2),        d: await Promise.resolve(3),        e: await Promise.resolve(4),    };}, "() => Promise<{ a: number; b: number; c: number; d: number; e: number; }>");

AssertType({        a: await Promise.resolve(0),        b: await Promise.resolve(1),        c: await Promise.resolve(2),        d: await Promise.resolve(3),        e: await Promise.resolve(4),    }, "{ a: number; b: number; c: number; d: number; e: number; }");
    return {

        a: await Promise.resolve(0),
AssertType(a, "number");
AssertType(await Promise.resolve(0), "number");
AssertType(Promise.resolve(0), "Promise<number>");
AssertType(Promise.resolve, "{ (): Promise<void>; <T>(T): Promise<Awaited<T>>; <T>(union): Promise<Awaited<T>>; }");
AssertType(0, "int");

        b: await Promise.resolve(1),
AssertType(b, "number");
AssertType(await Promise.resolve(1), "number");
AssertType(Promise.resolve(1), "Promise<number>");
AssertType(Promise.resolve, "{ (): Promise<void>; <T>(T): Promise<Awaited<T>>; <T>(union): Promise<Awaited<T>>; }");
AssertType(1, "int");

        c: await Promise.resolve(2),
AssertType(c, "number");
AssertType(await Promise.resolve(2), "number");
AssertType(Promise.resolve(2), "Promise<number>");
AssertType(Promise.resolve, "{ (): Promise<void>; <T>(T): Promise<Awaited<T>>; <T>(union): Promise<Awaited<T>>; }");
AssertType(2, "int");

        d: await Promise.resolve(3),
AssertType(d, "number");
AssertType(await Promise.resolve(3), "number");
AssertType(Promise.resolve(3), "Promise<number>");
AssertType(Promise.resolve, "{ (): Promise<void>; <T>(T): Promise<Awaited<T>>; <T>(union): Promise<Awaited<T>>; }");
AssertType(3, "int");

        e: await Promise.resolve(4),
AssertType(e, "number");
AssertType(await Promise.resolve(4), "number");
AssertType(Promise.resolve(4), "Promise<number>");
AssertType(Promise.resolve, "{ (): Promise<void>; <T>(T): Promise<Awaited<T>>; <T>(union): Promise<Awaited<T>>; }");
AssertType(4, "int");

    };
}

const fooLong = async () => {
AssertType(fooLong, "() => Promise<{ a: number; b: number; c: number; d: number; e: number; f: number; g: number; h: number; i: number; j: number; }>");
AssertType(async () => {    return {        a: await Promise.resolve(0),        b: await Promise.resolve(1),        c: await Promise.resolve(2),        d: await Promise.resolve(3),        e: await Promise.resolve(4),        f: await Promise.resolve(5),        g: await Promise.resolve(6),        h: await Promise.resolve(7),        i: await Promise.resolve(8),        j: await Promise.resolve(9),    };}, "() => Promise<{ a: number; b: number; c: number; d: number; e: number; f: number; g: number; h: number; i: number; j: number; }>");

AssertType({        a: await Promise.resolve(0),        b: await Promise.resolve(1),        c: await Promise.resolve(2),        d: await Promise.resolve(3),        e: await Promise.resolve(4),        f: await Promise.resolve(5),        g: await Promise.resolve(6),        h: await Promise.resolve(7),        i: await Promise.resolve(8),        j: await Promise.resolve(9),    }, "{ a: number; b: number; c: number; d: number; e: number; f: number; g: number; h: number; i: number; j: number; }");
    return {

        a: await Promise.resolve(0),
AssertType(a, "number");
AssertType(await Promise.resolve(0), "number");
AssertType(Promise.resolve(0), "Promise<number>");
AssertType(Promise.resolve, "{ (): Promise<void>; <T>(T): Promise<Awaited<T>>; <T>(union): Promise<Awaited<T>>; }");
AssertType(0, "int");

        b: await Promise.resolve(1),
AssertType(b, "number");
AssertType(await Promise.resolve(1), "number");
AssertType(Promise.resolve(1), "Promise<number>");
AssertType(Promise.resolve, "{ (): Promise<void>; <T>(T): Promise<Awaited<T>>; <T>(union): Promise<Awaited<T>>; }");
AssertType(1, "int");

        c: await Promise.resolve(2),
AssertType(c, "number");
AssertType(await Promise.resolve(2), "number");
AssertType(Promise.resolve(2), "Promise<number>");
AssertType(Promise.resolve, "{ (): Promise<void>; <T>(T): Promise<Awaited<T>>; <T>(union): Promise<Awaited<T>>; }");
AssertType(2, "int");

        d: await Promise.resolve(3),
AssertType(d, "number");
AssertType(await Promise.resolve(3), "number");
AssertType(Promise.resolve(3), "Promise<number>");
AssertType(Promise.resolve, "{ (): Promise<void>; <T>(T): Promise<Awaited<T>>; <T>(union): Promise<Awaited<T>>; }");
AssertType(3, "int");

        e: await Promise.resolve(4),
AssertType(e, "number");
AssertType(await Promise.resolve(4), "number");
AssertType(Promise.resolve(4), "Promise<number>");
AssertType(Promise.resolve, "{ (): Promise<void>; <T>(T): Promise<Awaited<T>>; <T>(union): Promise<Awaited<T>>; }");
AssertType(4, "int");

        f: await Promise.resolve(5),
AssertType(f, "number");
AssertType(await Promise.resolve(5), "number");
AssertType(Promise.resolve(5), "Promise<number>");
AssertType(Promise.resolve, "{ (): Promise<void>; <T>(T): Promise<Awaited<T>>; <T>(union): Promise<Awaited<T>>; }");
AssertType(5, "int");

        g: await Promise.resolve(6),
AssertType(g, "number");
AssertType(await Promise.resolve(6), "number");
AssertType(Promise.resolve(6), "Promise<number>");
AssertType(Promise.resolve, "{ (): Promise<void>; <T>(T): Promise<Awaited<T>>; <T>(union): Promise<Awaited<T>>; }");
AssertType(6, "int");

        h: await Promise.resolve(7),
AssertType(h, "number");
AssertType(await Promise.resolve(7), "number");
AssertType(Promise.resolve(7), "Promise<number>");
AssertType(Promise.resolve, "{ (): Promise<void>; <T>(T): Promise<Awaited<T>>; <T>(union): Promise<Awaited<T>>; }");
AssertType(7, "int");

        i: await Promise.resolve(8),
AssertType(i, "number");
AssertType(await Promise.resolve(8), "number");
AssertType(Promise.resolve(8), "Promise<number>");
AssertType(Promise.resolve, "{ (): Promise<void>; <T>(T): Promise<Awaited<T>>; <T>(union): Promise<Awaited<T>>; }");
AssertType(8, "int");

        j: await Promise.resolve(9),
AssertType(j, "number");
AssertType(await Promise.resolve(9), "number");
AssertType(Promise.resolve(9), "Promise<number>");
AssertType(Promise.resolve, "{ (): Promise<void>; <T>(T): Promise<Awaited<T>>; <T>(union): Promise<Awaited<T>>; }");
AssertType(9, "int");

    };
}


