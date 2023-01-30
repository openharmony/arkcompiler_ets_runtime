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

// === tests/cases/compiler/typePredicateFreshLiteralWidening.ts ===
declare function AssertType(value:any, type:string):void;
type Narrowable = string | number | bigint | boolean;

type Narrow<A> = (A extends Narrowable ? A : never) | ({
    [K in keyof A]: Narrow<A[K]>;
});

const satisfies =
AssertType(satisfies, "<TWide>() => <TNarrow extends TWide>(Narrow<TNarrow>) => Narrow<TNarrow>");

  <TWide,>() =>
AssertType(<TWide,>() =>  <TNarrow extends TWide>(narrow: Narrow<TNarrow>) =>    narrow, "<TWide>() => <TNarrow extends TWide>(Narrow<TNarrow>) => Narrow<TNarrow>");

  <TNarrow extends TWide>(narrow: Narrow<TNarrow>) =>
AssertType(<TNarrow extends TWide>(narrow: Narrow<TNarrow>) =>    narrow, "<TNarrow extends TWide>(Narrow<TNarrow>) => Narrow<TNarrow>");
AssertType(narrow, "Narrow<TNarrow>");

    narrow;
AssertType(narrow, "Narrow<TNarrow>");

/* ========================================================================== */

const isNotNull = <T,>(value: T | null): value is T => value !== null;
AssertType(isNotNull, "<T>(union) => value is T");
AssertType(<T,>(value: T | null): value is T => value !== null, "<T>(union) => value is T");
AssertType(value, "union");
AssertType(null, "null");
AssertType(value !== null, "boolean");
AssertType(value, "union");
AssertType(null, "null");

type Item = { value: string | null };

const item1 = satisfies<Item>()({ value: "1" });
AssertType(item1, "{ value: "1"; }");
AssertType(satisfies<Item>()({ value: "1" }), "{ value: "1"; }");
AssertType(satisfies<Item>(), "<TNarrow extends Item>(Narrow<TNarrow>) => Narrow<TNarrow>");
AssertType(satisfies, "<TWide>() => <TNarrow extends TWide>(Narrow<TNarrow>) => Narrow<TNarrow>");
AssertType({ value: "1" }, "{ value: "1"; }");
AssertType(value, "string");
AssertType("1", "string");

const item2 = satisfies<Item>()({ value: "2" });
AssertType(item2, "{ value: "2"; }");
AssertType(satisfies<Item>()({ value: "2" }), "{ value: "2"; }");
AssertType(satisfies<Item>(), "<TNarrow extends Item>(Narrow<TNarrow>) => Narrow<TNarrow>");
AssertType(satisfies, "<TWide>() => <TNarrow extends TWide>(Narrow<TNarrow>) => Narrow<TNarrow>");
AssertType({ value: "2" }, "{ value: "2"; }");
AssertType(value, "string");
AssertType("2", "string");

const item3 = satisfies<Item>()({ value: null });
AssertType(item3, "{ value: null; }");
AssertType(satisfies<Item>()({ value: null }), "{ value: null; }");
AssertType(satisfies<Item>(), "<TNarrow extends Item>(Narrow<TNarrow>) => Narrow<TNarrow>");
AssertType(satisfies, "<TWide>() => <TNarrow extends TWide>(Narrow<TNarrow>) => Narrow<TNarrow>");
AssertType({ value: null }, "{ value: null; }");
AssertType(value, "null");
AssertType(null, "null");

const values2: Array<"1" | "2" | null> = ["1", "2", null];
AssertType(values2, "(union)[]");
AssertType(null, "null");
AssertType(["1", "2", null], "(union)[]");
AssertType("1", "string");
AssertType("2", "string");
AssertType(null, "null");

const filteredValues2 = values2.filter(isNotNull);
AssertType(filteredValues2, "(union)[]");
AssertType(values2.filter(isNotNull), "(union)[]");
AssertType(values2.filter, "{ <S extends union>((union, number, (union)[]) => value is S, ?any): S[]; ((union, number, (union)[]) => unknown, ?any): (union)[]; }");
AssertType(isNotNull, "<T>(union) => value is T");

const values1 = [item1, item2, item3].map(item => item.value);
AssertType(values1, "(union)[]");
AssertType([item1, item2, item3].map(item => item.value), "(union)[]");
AssertType([item1, item2, item3].map, "<U>((union, number, (union)[]) => U, ?any) => U[]");
AssertType(item => item.value, "(union) => union");
AssertType(item, "union");
AssertType(item.value, "union");

const filteredValues1 = values1.filter(isNotNull);
AssertType(filteredValues1, "(union)[]");
AssertType(values1.filter(isNotNull), "(union)[]");
AssertType(values1.filter, "{ <S extends union>((union, number, (union)[]) => value is S, ?any): S[]; ((union, number, (union)[]) => unknown, ?any): (union)[]; }");
AssertType(isNotNull, "<T>(union) => value is T");


