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

// === tests/cases/conformance/classes/members/classTypes/indexersInClassType.ts ===
declare function AssertType(value:any, type:string):void;
class C {
    [x: number]: Date;
    [x: string]: Object;
    1: Date;
    'a': {}

    fn() {
AssertType(this, "this");
        return this;
    }
}

let c = new C();
AssertType(c, "C");
AssertType(new C(), "C");
AssertType(C, "typeof C");

let r = c.fn();
AssertType(r, "C");
AssertType(c.fn(), "C");
AssertType(c.fn, "() => C");

let r2 = r[1];
AssertType(r2, "Date");
AssertType(r[1], "Date");
AssertType(r, "C");
AssertType(1, "int");

let r3 = r.a
AssertType(r3, "{}");
AssertType(r.a, "{}");



