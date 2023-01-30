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

// === tests/cases/compiler/inferenceErasedSignatures.ts ===
declare function AssertType(value:any, type:string):void;
// Repro from #37163

declare class SomeBaseClass {
    set<K extends keyof this>(key: K, value: this[K]): this[K];
}

abstract class SomeAbstractClass<C, M, R> extends SomeBaseClass {
    foo!: (r?: R) => void;
    bar!: (r?: any) => void;
    abstract baz(c: C): Promise<M>;
}

class SomeClass extends SomeAbstractClass<number, string, boolean> {
    async baz(context: number): Promise<string> {
AssertType(`${context}`, "string");
AssertType(context, "number");
        return `${context}`;
    }
}

type CType<T> = T extends SomeAbstractClass<infer C, any, any> ? C : never;
type MType<T> = T extends SomeAbstractClass<any, infer M, any> ? M : never;
type RType<T> = T extends SomeAbstractClass<any, any, infer R> ? R : never;

type SomeClassC = CType<SomeClass>; // number
type SomeClassM = MType<SomeClass>; // string
type SomeClassR = RType<SomeClass>; // boolean

// Repro from #37163

interface BaseType<T1, T2>  {
    set<K extends keyof this>(key: K, value: this[K]): this[K];
    useT1(c: T1): void;
    useT2(r?: T2): void;
    unrelatedButSomehowRelevant(r?: any): void;
}

interface InheritedType extends BaseType<number, boolean> {
    // This declaration shouldn't do anything...
    useT1(_: number): void
}

// Structural expansion of InheritedType
interface StructuralVersion  {
    set<K extends keyof this>(key: K, value: this[K]): this[K];
    useT1(c: number): void;
    useT2(r?: boolean): void;
    unrelatedButSomehowRelevant(r?: any): void;
}

type GetT1<T> = T extends BaseType<infer U, any> ? U : never;

type T1 = GetT1<InheritedType>; // number
type T2 = GetT1<StructuralVersion>; // number


