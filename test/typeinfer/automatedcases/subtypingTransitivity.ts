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

// === tests/cases/compiler/subtypingTransitivity.ts ===
declare function AssertType(value:any, type:string):void;
class B {
    x: Object;
}

class D extends B {
    public x: string;
}
class D2 extends B {
    public x: number;
}

let b: B;
AssertType(b, "B");

let d: D;
AssertType(d, "D");

let d2: D2;
AssertType(d2, "D2");

d.x = '';
AssertType(d.x = '', "string");
AssertType(d.x, "string");
AssertType('', "string");

b = d;
AssertType(b = d, "D");
AssertType(b, "B");
AssertType(d, "D");

b.x = 1; // assigned number to string
AssertType(b.x = 1, "int");
AssertType(b.x, "Object");
AssertType(1, "int");


