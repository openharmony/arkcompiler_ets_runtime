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

// === tests/cases/compiler/narrowingConstrainedTypeParameter.ts ===
declare function AssertType(value:any, type:string):void;
// Repro from #10811

interface Pet {
    name: string;
}

function isPet(pet: any): pet is Pet {
AssertType(typeof pet.name === "string", "boolean");
AssertType(typeof pet.name, "union");
AssertType(pet.name, "any");
AssertType("string", "string");
    return typeof pet.name === "string";
}

export function speak<TPet extends Pet>(pet: TPet, voice: (pet: TPet) => string): string {
    if (!isPet(pet)) {
AssertType(!isPet(pet), "boolean");
AssertType(isPet(pet), "boolean");
AssertType(isPet, "(any) => pet is Pet");
AssertType(pet, "TPet");

        throw new Error("Expected \"pet\" to be a Pet");
AssertType(new Error("Expected \"pet\" to be a Pet"), "Error");
AssertType(Error, "ErrorConstructor");
AssertType("Expected \"pet\" to be a Pet", "string");
    }
AssertType(voice(pet), "string");
AssertType(voice, "(TPet) => string");
AssertType(pet, "TPet");
    return voice(pet);
}

