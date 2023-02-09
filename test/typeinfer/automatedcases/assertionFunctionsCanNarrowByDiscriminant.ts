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

// === tests/cases/compiler/assertionFunctionsCanNarrowByDiscriminant.ts ===
declare function AssertType(value:any, type:string):void;
interface Cat {
    type: 'cat';
    canMeow: true;
}

interface Dog {
    type: 'dog';
    canBark: true;
}

type Animal = Cat | Dog;

declare function assertEqual<T>(value: any, type: T): asserts value is T;

const animal = { type: 'cat', canMeow: true } as Animal;
AssertType(animal, "Animal");
AssertType({ type: 'cat', canMeow: true } as Animal, "Animal");
AssertType({ type: 'cat', canMeow: true }, "{ type: "cat"; canMeow: true; }");
AssertType(type, "string");
AssertType('cat', "string");
AssertType(canMeow, "boolean");
AssertType(true, "boolean");

assertEqual(animal.type, 'cat' as const);
AssertType(assertEqual(animal.type, 'cat' as const), "void");
AssertType(assertEqual, "<T>(any, T) => asserts value is T");
AssertType(animal.type, "union");
AssertType('cat' as const, "string");
AssertType('cat', "string");

animal.canMeow; // since is cat, should not be an error
AssertType(animal.canMeow, "boolean");

const animalOrUndef = { type: 'cat', canMeow: true } as Animal | undefined;
AssertType(animalOrUndef, "union");
AssertType({ type: 'cat', canMeow: true } as Animal | undefined, "union");
AssertType({ type: 'cat', canMeow: true }, "{ type: "cat"; canMeow: true; }");
AssertType(type, "string");
AssertType('cat', "string");
AssertType(canMeow, "boolean");
AssertType(true, "boolean");

assertEqual(animalOrUndef?.type, 'cat' as const);
AssertType(assertEqual(animalOrUndef?.type, 'cat' as const), "void");
AssertType(assertEqual, "<T>(any, T) => asserts value is T");
AssertType(animalOrUndef?.type, "union");
AssertType('cat' as const, "string");
AssertType('cat', "string");

animalOrUndef.canMeow; // since is cat, should not be an error
AssertType(animalOrUndef.canMeow, "boolean");


