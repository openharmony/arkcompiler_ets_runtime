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

// === tests/cases/compiler/nonInferrableTypePropagation1.ts ===
declare function AssertType(value:any, type:string):void;
type Op<I, O> = (thing: Thing<I>) => Thing<O>;
type Thing<T> = {
    value: T;
    pipe<A, B>(
        opA: Op<T, A>,
        opB: Op<A, B>,
    ): Thing<B>;
};
type Box<V> = { data: V };

declare const thing: Thing<number>;
AssertType(thing, "Thing<number>");

declare function map<T, R>(project: (value: T) => R): Op<T, R>;
declare function tap<T>(next: (value: T) => void): Op<T, T>;
declare function box<V>(data: V): Box<V>;
declare function createAndUnbox<V>(factory: () => Thing<V | Box<V>>): Thing<V>;
declare function log(value: any): void;

const result1 = createAndUnbox(() => thing.pipe(
AssertType(result1, "Thing<number>");
AssertType(createAndUnbox(() => thing.pipe(    map((data) => box(data)),    tap((v) => log(v)),)), "Thing<number>");
AssertType(createAndUnbox, "<V>(() => Thing<union>) => Thing<V>");
AssertType(() => thing.pipe(    map((data) => box(data)),    tap((v) => log(v)),), "() => Thing<Box<number>>");
AssertType(thing.pipe(    map((data) => box(data)),    tap((v) => log(v)),), "Thing<Box<number>>");
AssertType(thing.pipe, "<A, B>(Op<number, A>, Op<A, B>) => Thing<B>");

    map((data) => box(data)),
AssertType(map((data) => box(data)), "Op<number, Box<number>>");
AssertType(map, "<T, R>((T) => R) => Op<T, R>");
AssertType((data) => box(data), "(number) => Box<number>");
AssertType(data, "number");
AssertType(box(data), "Box<number>");
AssertType(box, "<V>(V) => Box<V>");
AssertType(data, "number");

    tap((v) => log(v)),
AssertType(tap((v) => log(v)), "Op<Box<number>, Box<number>>");
AssertType(tap, "<T>((T) => void) => Op<T, T>");
AssertType((v) => log(v), "(Box<number>) => void");
AssertType(v, "Box<number>");
AssertType(log(v), "void");
AssertType(log, "(any) => void");
AssertType(v, "Box<number>");

));

const result2 = createAndUnbox(() => thing.pipe(
AssertType(result2, "Thing<number>");
AssertType(createAndUnbox(() => thing.pipe(    tap((v) => log(v)),    map((data) => box(data)),)), "Thing<number>");
AssertType(createAndUnbox, "<V>(() => Thing<union>) => Thing<V>");
AssertType(() => thing.pipe(    tap((v) => log(v)),    map((data) => box(data)),), "() => Thing<Box<number>>");
AssertType(thing.pipe(    tap((v) => log(v)),    map((data) => box(data)),), "Thing<Box<number>>");
AssertType(thing.pipe, "<A, B>(Op<number, A>, Op<A, B>) => Thing<B>");

    tap((v) => log(v)),
AssertType(tap((v) => log(v)), "Op<number, number>");
AssertType(tap, "<T>((T) => void) => Op<T, T>");
AssertType((v) => log(v), "(number) => void");
AssertType(v, "number");
AssertType(log(v), "void");
AssertType(log, "(any) => void");
AssertType(v, "number");

    map((data) => box(data)),
AssertType(map((data) => box(data)), "Op<number, Box<number>>");
AssertType(map, "<T, R>((T) => R) => Op<T, R>");
AssertType((data) => box(data), "(number) => Box<number>");
AssertType(data, "number");
AssertType(box(data), "Box<number>");
AssertType(box, "<V>(V) => Box<V>");
AssertType(data, "number");

));


