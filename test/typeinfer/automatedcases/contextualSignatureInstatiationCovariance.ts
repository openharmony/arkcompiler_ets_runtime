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

// === tests/cases/compiler/contextualSignatureInstatiationColetiance.ts ===
declare function AssertType(value:any, type:string):void;
interface Animal { x }
interface TallThing { x2 }
interface Giraffe extends Animal, TallThing { y }

let f2: <T extends Giraffe>(x: T, y: T) => void;
AssertType(f2, "<T extends Giraffe>(T, T) => void");
AssertType(x, "T");
AssertType(y, "T");

let g2: (a: Animal, t: TallThing) => void;
AssertType(g2, "(Animal, TallThing) => void");
AssertType(a, "Animal");
AssertType(t, "TallThing");

g2 = f2; // While neither Animal nor TallThing satisfy the constraint, T is at worst a Giraffe and compatible with both via coletiance.
AssertType(g2 = f2, "<T extends Giraffe>(T, T) => void");
AssertType(g2, "(Animal, TallThing) => void");
AssertType(f2, "<T extends Giraffe>(T, T) => void");

let h2: (a1: Animal, a2: Animal) => void;
AssertType(h2, "(Animal, Animal) => void");
AssertType(a1, "Animal");
AssertType(a2, "Animal");

h2 = f2; // Animal does not satisfy the constraint, but T is at worst a Giraffe and compatible with Animal via coletiance.
AssertType(h2 = f2, "<T extends Giraffe>(T, T) => void");
AssertType(h2, "(Animal, Animal) => void");
AssertType(f2, "<T extends Giraffe>(T, T) => void");


