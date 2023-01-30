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

// === tests/cases/compiler/superWithGenericSpecialization.ts ===
declare function AssertType(value:any, type:string):void;
class C<T> {
    x: T;
}

class D<T> extends C<string> {
    y: T;
    constructor() {
        super(); // uses the type parameter type of the base class, ie string
AssertType(super(), "void");
AssertType(super, "typeof C");
    }
}

let d: D<number>;
AssertType(d, "D<number>");

let r: string = d.x;
AssertType(r, "string");
AssertType(d.x, "string");

let r2: number = d.y;
AssertType(r2, "number");
AssertType(d.y, "number");


