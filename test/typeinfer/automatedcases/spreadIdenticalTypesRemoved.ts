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

// === tests/cases/compiler/spreadIdenticalTypesRemoved.ts ===
declare function AssertType(value:any, type:string):void;
interface Animal {
    name: string;
    kind: string;
    age: number;
    location: string;
    owner: object;
}

function clonePet(pet: Animal, fullCopy?: boolean) {
AssertType({        name: pet.name,        kind: pet.kind,        ...(fullCopy && pet),    }, "{ name: string; kind: string; age?: union; location?: union; owner?: union; }");
    return {

        name: pet.name,
AssertType(name, "string");
AssertType(pet.name, "string");

        kind: pet.kind,
AssertType(kind, "string");
AssertType(pet.kind, "string");

        ...(fullCopy && pet),
AssertType((fullCopy && pet), "union");
AssertType(fullCopy && pet, "union");
AssertType(fullCopy, "union");
AssertType(pet, "Animal");
    }
}

interface Animal2 {
    name: string;
    owner?: string;
}
function billOwner(pet: Animal2) {
AssertType({        ...(pet.owner && pet),        paid: false    }, "{ paid: boolean; name?: union; owner?: union; }");
    return {

        ...(pet.owner && pet),
AssertType((pet.owner && pet), "union");
AssertType(pet.owner && pet, "union");
AssertType(pet.owner, "union");
AssertType(pet, "Animal2");

        paid: false
AssertType(paid, "boolean");
AssertType(false, "boolean");
    }
}


