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

// === tests/cases/compiler/objectInstantiationFromUnionSpread.ts ===
declare function AssertType(value:any, type:string):void;
// #40995

interface Success {
  isSuccess: true;
}

interface Fail {
  isSuccess: false;
}

type Item = Success | Fail;

function f1(a: Item[]) {
  a.map(item => ({ ...item })).filter(value => {});
AssertType(a.map(item => ({ ...item })).filter(value => {}), "(union)[]");
AssertType(a.map(item => ({ ...item })).filter, "{ <S extends union>((union, number, (union)[]) => value is S, ?any): S[]; ((union, number, (union)[]) => unknown, ?any): (union)[]; }");
AssertType(a.map(item => ({ ...item })), "(union)[]");
AssertType(a.map, "<U>((Item, number, Item[]) => U, ?any) => U[]");
AssertType(item => ({ ...item }), "(Item) => union");
AssertType(item, "Item");
AssertType(({ ...item }), "union");
AssertType({ ...item }, "union");
AssertType(item, "Item");
AssertType(value => {}, "(union) => void");
AssertType(value, "union");
}

function f2<T>(a: Item[]) {
  a.map(item => ({ ...item })).filter(value => {});
AssertType(a.map(item => ({ ...item })).filter(value => {}), "(union)[]");
AssertType(a.map(item => ({ ...item })).filter, "{ <S extends union>((union, number, (union)[]) => value is S, ?any): S[]; ((union, number, (union)[]) => unknown, ?any): (union)[]; }");
AssertType(a.map(item => ({ ...item })), "(union)[]");
AssertType(a.map, "<U>((Item, number, Item[]) => U, ?any) => U[]");
AssertType(item => ({ ...item }), "(Item) => union");
AssertType(item, "Item");
AssertType(({ ...item }), "union");
AssertType({ ...item }, "union");
AssertType(item, "Item");
AssertType(value => {}, "(union) => void");
AssertType(value, "union");
}


