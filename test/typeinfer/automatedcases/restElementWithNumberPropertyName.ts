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

// === tests/cases/compiler/restElementWithNumberPropertyName.ts ===
declare function AssertType(value:any, type:string):void;
const { 0: a, ...b } = [0, 1, 2];
AssertType(a, "number");
AssertType(b, "{ [number]: number; 0: number; 1: number; 2: number; length: 3; toString(): string; toLocaleString(): string; pop(): number; push(...number[]): number; concat(...ConcatArray<number>[]): number[]; concat(...(union)[]): number[]; join(?string): string; reverse(): number[]; shift(): number; slice(?number, ?number): number[]; sort(?(number, number) => number): [number, number, number]; splice(number, ?number): number[]; splice(number, number, ...number[]): number[]; unshift(...number[]): number; indexOf(number, ?number): number; lastIndexOf(number, ?number): number; every<S extends number>((number, number, number[]) => value is S, ?any): this is S[]; every((number, number, number[]) => unknown, ?any): boolean; some((number, number, number[]) => unknown, ?any): boolean; forEach((number, number, number[]) => void, ?any): void; map<U>((number, number, number[]) => U, ?any): U[]; filter<S extends number>((number, number, number[]) => value is S, ?any): S[]; filter((number, number, number[]) => unknown, ?any): number[]; reduce((number, number, number, number[]) => number): number; reduce((number, number, number, number[]) => number, number): number; reduce<U>((U, number, number, number[]) => U, U): U; reduceRight((number, number, number, number[]) => number): number; reduceRight((number, number, number, number[]) => number, number): number; reduceRight<U>((U, number, number, number[]) => U, U): U; }");
AssertType([0, 1, 2], "[number, number, number]");
AssertType(0, "int");
AssertType(1, "int");
AssertType(2, "int");


