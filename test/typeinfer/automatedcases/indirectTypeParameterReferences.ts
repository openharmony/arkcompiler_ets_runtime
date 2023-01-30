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

// === tests/cases/compiler/indirectTypeParameterReferences.ts ===
declare function AssertType(value:any, type:string):void;
// Repro from #19043

type B = {b: string}

const flowtypes = <A>(b: B) => {
AssertType(flowtypes, "<A>(B) => { combined: ((A & B) => void) => any; literal: ((A & B) => void) => any; }");
AssertType(b, "B");
AssertType(<A>(b: B) => {  type Combined = A & B  const combined = (fn: (combined: Combined) => void) => null  const literal = (fn: (aPlusB: A & B) => void) => null  return {combined, literal}}, "<A>(B) => { combined: ((A & B) => void) => any; literal: ((A & B) => void) => any; }");

  type Combined = A & B
AssertType(Combined, "A & B");

  const combined = (fn: (combined: Combined) => void) => null
AssertType(combined, "((A & B) => void) => any");
AssertType((fn: (combined: Combined) => void) => null, "((A & B) => void) => any");
AssertType(fn, "(A & B) => void");
AssertType(combined, "A & B");
AssertType(null, "null");

  const literal = (fn: (aPlusB: A & B) => void) => null
AssertType(literal, "((A & B) => void) => any");
AssertType((fn: (aPlusB: A & B) => void) => null, "((A & B) => void) => any");
AssertType(fn, "(A & B) => void");
AssertType(aPlusB, "A & B");
AssertType(null, "null");

AssertType({combined, literal}, "{ combined: ((A & B) => void) => any; literal: ((A & B) => void) => any; }");
  return {combined, literal

AssertType(combined, "((A & B) => void) => any");

AssertType(literal, "((A & B) => void) => any");
}
}

const {combined, literal} = flowtypes<{a: string}>({b: 'b-value'})
AssertType(combined, "(({ a: string; } & B) => void) => any");
AssertType(literal, "(({ a: string; } & B) => void) => any");
AssertType(flowtypes<{a: string}>({b: 'b-value'}), "{ combined: (({ a: string; } & B) => void) => any; literal: (({ a: string; } & B) => void) => any; }");
AssertType(flowtypes, "<A>(B) => { combined: ((A & B) => void) => any; literal: ((A & B) => void) => any; }");
AssertType(a, "string");
AssertType({b: 'b-value'}, "{ b: string; }");
AssertType(b, "string");
AssertType('b-value', "string");

literal(aPlusB => {
AssertType(literal(aPlusB => {  aPlusB.b  aPlusB.a}), "any");
AssertType(literal, "(({ a: string; } & B) => void) => any");
AssertType(aPlusB => {  aPlusB.b  aPlusB.a}, "({ a: string; } & B) => void");
AssertType(aPlusB, "{ a: string; } & B");

  aPlusB.b
AssertType(aPlusB.b, "string");

  aPlusB.a
AssertType(aPlusB.a, "string");

})

combined(comb => {
AssertType(combined(comb => {  comb.b  comb.a}), "any");
AssertType(combined, "(({ a: string; } & B) => void) => any");
AssertType(comb => {  comb.b  comb.a}, "({ a: string; } & B) => void");
AssertType(comb, "{ a: string; } & B");

  comb.b
AssertType(comb.b, "string");

  comb.a
AssertType(comb.a, "string");

})

// Repro from #19091

declare function f<T>(a: T): { a: typeof a };
let n: number = f(2).a;
AssertType(n, "number");
AssertType(f(2).a, "number");


