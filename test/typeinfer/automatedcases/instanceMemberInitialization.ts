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

// === tests/cases/conformance/classes/propertyMemberDeclarations/instanceMemberInitialization.ts ===
declare function AssertType(value:any, type:string):void;
class C {
    x = 1;
}

let c = new C();
AssertType(c, "C");
AssertType(new C(), "C");
AssertType(C, "typeof C");

c.x = 3;
AssertType(c.x = 3, "int");
AssertType(c.x, "number");
AssertType(3, "int");

let c2 = new C();
AssertType(c2, "C");
AssertType(new C(), "C");
AssertType(C, "typeof C");

let r = c.x === c2.x;
AssertType(r, "boolean");
AssertType(c.x === c2.x, "boolean");
AssertType(c.x, "number");
AssertType(c2.x, "number");

// #31792



class MyMap<K, V> {
    constructor(private readonly Map_: { new<K, V>(): any }) {}
    private readonly store = new this.Map_<K, V>();
}

