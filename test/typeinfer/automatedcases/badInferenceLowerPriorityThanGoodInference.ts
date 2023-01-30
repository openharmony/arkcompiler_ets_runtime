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

// === tests/cases/compiler/badInferenceLowerPriorityThanGoodInference.ts ===
declare function AssertType(value:any, type:string):void;
// Repro from #13118

interface Foo<A> {
    a: A;
    b: (x: A) => void;
}

declare function canYouInferThis<A>(fn: () => Foo<A>): A;

const result = canYouInferThis(() => ({
AssertType(result, "{ BLAH: number; }");
AssertType(canYouInferThis(() => ({    a: { BLAH: 33 },    b: x => { }})), "{ BLAH: number; }");
AssertType(canYouInferThis, "<A>(() => Foo<A>) => A");
AssertType(() => ({    a: { BLAH: 33 },    b: x => { }}), "() => { a: { BLAH: number; }; b: ({ BLAH: number; }) => void; }");
AssertType(({    a: { BLAH: 33 },    b: x => { }}), "{ a: { BLAH: number; }; b: ({ BLAH: number; }) => void; }");
AssertType({    a: { BLAH: 33 },    b: x => { }}, "{ a: { BLAH: number; }; b: ({ BLAH: number; }) => void; }");

    a: { BLAH: 33 },
AssertType(a, "{ BLAH: number; }");
AssertType({ BLAH: 33 }, "{ BLAH: number; }");
AssertType(BLAH, "number");
AssertType(33, "int");

    b: x => { 
AssertType(b, "({ BLAH: number; }) => void");

AssertType(x => { }, "({ BLAH: number; }) => void");

AssertType(x, "{ BLAH: number; }");
}

}))

result.BLAH;
AssertType(result.BLAH, "number");

// Repro from #26629

function goofus <ARGS extends any[]> (f: (...args: ARGS) => any ) {}

goofus((a: string) => ({ dog() { 
AssertType(goofus((a: string) => ({ dog() { return a; } })), "void");
AssertType(goofus, "<ARGS extends any[]>((...ARGS) => any) => void");
AssertType((a: string) => ({ dog() { return a; } }), "(string) => { dog(): string; }");
AssertType(a, "string");
AssertType(({ dog() { return a; } }), "{ dog(): string; }");
AssertType({ dog() { return a; } }, "{ dog(): string; }");
AssertType(dog, "() => string");
AssertType(a, "string");
return a; } }));

goofus((a: string) => ({ dog: function() { 
AssertType(goofus((a: string) => ({ dog: function() { return a; } })), "void");
AssertType(goofus, "<ARGS extends any[]>((...ARGS) => any) => void");
AssertType((a: string) => ({ dog: function() { return a; } }), "(string) => { dog: () => string; }");
AssertType(a, "string");
AssertType(({ dog: function() { return a; } }), "{ dog: () => string; }");
AssertType({ dog: function() { return a; } }, "{ dog: () => string; }");
AssertType(dog, "() => string");
AssertType(function() { return a; }, "() => string");
AssertType(a, "string");
return a; } }));


