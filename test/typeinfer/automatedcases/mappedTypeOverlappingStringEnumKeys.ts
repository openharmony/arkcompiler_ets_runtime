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

// === tests/cases/conformance/types/mapped/mappedTypeOverlappingStringEnumKeys.ts ===
declare function AssertType(value:any, type:string):void;
// #37859

enum TerrestrialAnimalTypes {
  CAT = "cat",
  DOG = "dog"
};

enum AlienAnimalTypes {
  CAT = "cat",
};

type AnimalTypes = TerrestrialAnimalTypes | AlienAnimalTypes;

interface TerrestrialCat {
  type: TerrestrialAnimalTypes.CAT;
  address: string;
}

interface AlienCat {
  type: AlienAnimalTypes.CAT
  planet: string;
}

type Cats = TerrestrialCat | AlienCat;

type CatMap = {
  [V in AnimalTypes]: Extract<Cats, { type: V }>[]
};

const catMap: CatMap = {
AssertType(catMap, "CatMap");
AssertType({  cat: [    { type: TerrestrialAnimalTypes.CAT, address: "" },    { type: AlienAnimalTypes.CAT, planet: "" }  ],  dog: [] as never[]}, "{ cat: (union)[]; dog: never[]; }");

  cat: [
AssertType(cat, "(union)[]");
AssertType([    { type: TerrestrialAnimalTypes.CAT, address: "" },    { type: AlienAnimalTypes.CAT, planet: "" }  ], "(union)[]");

    { type: TerrestrialAnimalTypes.CAT, address: "" },
AssertType({ type: TerrestrialAnimalTypes.CAT, address: "" }, "{ type: TerrestrialAnimalTypes.CAT; address: string; }");
AssertType(type, "TerrestrialAnimalTypes.CAT");
AssertType(TerrestrialAnimalTypes.CAT, "TerrestrialAnimalTypes.CAT");
AssertType(address, "string");
AssertType("", "string");

    { type: AlienAnimalTypes.CAT, planet: "" 
AssertType({ type: AlienAnimalTypes.CAT, planet: "" }, "{ type: AlienAnimalTypes; planet: string; }");

AssertType(type, "AlienAnimalTypes");

AssertType(AlienAnimalTypes.CAT, "AlienAnimalTypes");

AssertType(planet, "string");

AssertType("", "string");
}

  ],
  dog: [] as never[]
AssertType(dog, "never[]");
AssertType([] as never[], "never[]");
AssertType([], "undefined[]");

};


