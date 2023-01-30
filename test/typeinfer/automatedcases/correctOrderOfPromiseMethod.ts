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

// === tests/cases/compiler/correctOrderOfPromiseMethod.ts ===
declare function AssertType(value:any, type:string):void;
interface A {
    id: string
}

interface B {
    id: string
    fieldB: string
}

async function countEverything(): Promise<number> {
    const providerA = async (): Promise<A[]> => { 
AssertType(providerA, "() => Promise<A[]>");
return [] 

AssertType(async (): Promise<A[]> => { return [] }, "() => Promise<A[]>");

AssertType([], "undefined[]");
}

    const providerB = async (): Promise<B[]> => { 
AssertType(providerB, "() => Promise<B[]>");
return [] 

AssertType(async (): Promise<B[]> => { return [] }, "() => Promise<B[]>");

AssertType([], "undefined[]");
}

    const [resultA, resultB] = await Promise.all([
AssertType(resultA, "A[]");
AssertType(resultB, "B[]");
AssertType(await Promise.all([        providerA(),        providerB(),    ]), "[A[], B[]]");
AssertType(Promise.all([        providerA(),        providerB(),    ]), "Promise<[A[], B[]]>");
AssertType(Promise.all, "{ <T>(Iterable<union>): Promise<Awaited<T>[]>; <T extends union>(T): Promise<{ -readonly [P in keyof T]: Awaited<T[P]>; }>; }");
AssertType([        providerA(),        providerB(),    ], "[Promise<A[]>, Promise<B[]>]");

        providerA(),
AssertType(providerA(), "Promise<A[]>");
AssertType(providerA, "() => Promise<A[]>");

        providerB(),
AssertType(providerB(), "Promise<B[]>");
AssertType(providerB, "() => Promise<B[]>");

    ]);

    const dataA: A[] = resultA;
AssertType(dataA, "A[]");
AssertType(resultA, "A[]");

    const dataB: B[] = resultB;
AssertType(dataB, "B[]");
AssertType(resultB, "B[]");

    if (dataA && dataB) {
AssertType(dataA && dataB, "B[]");
AssertType(dataA, "A[]");
AssertType(dataB, "B[]");

AssertType(dataA.length + dataB.length, "number");
AssertType(dataA.length, "number");
AssertType(dataB.length, "number");
        return dataA.length + dataB.length;
    }
AssertType(0, "int");
    return 0;
}

// #31179

const expected: Promise<["a", "b", "c"]> = Promise.all(undefined as readonly ["a", "b", "c"]);
AssertType(expected, "Promise<["a", "b", "c"]>");
AssertType(Promise.all(undefined as readonly ["a", "b", "c"]), "Promise<["a", "b", "c"]>");
AssertType(Promise.all, "{ <T>(Iterable<union>): Promise<Awaited<T>[]>; <T extends union>(T): Promise<{ -readonly [P in keyof T]: Awaited<T[P]>; }>; }");
AssertType(undefined as readonly ["a", "b", "c"], "readonly ["a", "b", "c"]");
AssertType(undefined, "undefined");


