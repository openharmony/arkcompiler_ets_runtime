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

// === tests/cases/conformance/types/stringLiteral/stringLiteralTypesAndTuples01.ts ===
declare function AssertType(value:any, type:string):void;
// Should all be strings.
let [hello, brave, newish, world] = ["Hello", "Brave", "New", "World"];
AssertType(hello, "string");
AssertType(brave, "string");
AssertType(newish, "string");
AssertType(world, "string");
AssertType(["Hello", "Brave", "New", "World"], "[string, string, string, string]");
AssertType("Hello", "string");
AssertType("Brave", "string");
AssertType("New", "string");
AssertType("World", "string");

type RexOrRaptor = "t-rex" | "raptor"
let [im, a, dinosaur]: ["I'm", "a", RexOrRaptor] = ['I\'m', 'a', 't-rex'];
AssertType(im, "string");
AssertType(a, "string");
AssertType(dinosaur, "RexOrRaptor");
AssertType(['I\'m', 'a', 't-rex'], "["I'm", "a", "t-rex"]");
AssertType('I\'m', "string");
AssertType('a', "string");
AssertType('t-rex', "string");

rawr(dinosaur);
AssertType(rawr(dinosaur), "union");
AssertType(rawr, "(RexOrRaptor) => union");
AssertType(dinosaur, "string");

function rawr(dino: RexOrRaptor) {
    if (dino === "t-rex") {
AssertType(dino === "t-rex", "boolean");
AssertType(dino, "RexOrRaptor");
AssertType("t-rex", "string");

AssertType("ROAAAAR!", "string");
        return "ROAAAAR!";
    }
    if (dino === "raptor") {
AssertType(dino === "raptor", "boolean");
AssertType(dino, "string");
AssertType("raptor", "string");

AssertType("yip yip!", "string");
        return "yip yip!";
    }

    throw "Unexpected " + dino;
AssertType("Unexpected " + dino, "string");
AssertType("Unexpected ", "string");
AssertType(dino, "never");
}

