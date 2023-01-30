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

// === tests/cases/compiler/staticPrototypePropertyOnClass.ts ===
declare function AssertType(value:any, type:string):void;
class c1 {
}
class c2<T> {
}
class c3 {
    constructor() {
    }
}
class c4 {
    constructor(param: string);
    constructor(param: number);
    constructor(param: any) {
    }
}
let a = c1;
AssertType(a, "typeof c1");
AssertType(c1, "typeof c1");

let b = c2;
AssertType(b, "typeof c2");
AssertType(c2, "typeof c2");

let c = c3;
AssertType(c, "typeof c3");
AssertType(c3, "typeof c3");

let d = c4;
AssertType(d, "typeof c4");
AssertType(c4, "typeof c4");


