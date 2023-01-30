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

// === tests/cases/compiler/prespecializedGenericMembers1.ts ===
declare function AssertType(value:any, type:string):void;
export interface IKitty {

    }

export class Cat<CatType extends IKitty> {
    constructor() {
    
    }
}

export class CatBag {
    constructor(cats: { barry: Cat<IKitty>; }) {
        
    }
}
let cat = new Cat<IKitty>();
AssertType(cat, "Cat<IKitty>");
AssertType(new Cat<IKitty>(), "Cat<IKitty>");
AssertType(Cat, "typeof Cat");

let catThing = {
AssertType(catThing, "{ barry: Cat<IKitty>; }");
AssertType({    barry: cat}, "{ barry: Cat<IKitty>; }");

    barry: cat
AssertType(barry, "Cat<IKitty>");
AssertType(cat, "Cat<IKitty>");

};
let catBag = new CatBag(catThing);
AssertType(catBag, "CatBag");
AssertType(new CatBag(catThing), "CatBag");
AssertType(CatBag, "typeof CatBag");
AssertType(catThing, "{ barry: Cat<IKitty>; }");


