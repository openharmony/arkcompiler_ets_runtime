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

// === tests/cases/conformance/classes/members/instanceAndStaticMembers/typeOfThisInStaticMembers.ts ===
declare function AssertType(value:any, type:string):void;
class C {
    constructor(x: number) { }
    static foo: number;
    static bar() {
        // type of this is the constructor function type
        let t = this;
AssertType(t, "typeof C");
AssertType(this, "typeof C");

AssertType(this, "typeof C");
        return this;
    }
}

let t = C.bar();
AssertType(t, "typeof C");
AssertType(C.bar(), "typeof C");
AssertType(C.bar, "() => typeof C");

// all ok
let r2 = t.foo + 1;
AssertType(r2, "number");
AssertType(t.foo + 1, "number");
AssertType(t.foo, "number");
AssertType(1, "int");

let r3 = t.bar();
AssertType(r3, "typeof C");
AssertType(t.bar(), "typeof C");
AssertType(t.bar, "() => typeof C");

let r4 = new t(1);
AssertType(r4, "C");
AssertType(new t(1), "C");
AssertType(t, "typeof C");
AssertType(1, "int");

class C2<T> {
    static test: number;
    constructor(x: string) { }
    static foo: string;
    static bar() {
        // type of this is the constructor function type
        let t = this;
AssertType(t, "typeof C2");
AssertType(this, "typeof C2");

AssertType(this, "typeof C2");
        return this;
    }
}

let t2 = C2.bar();
AssertType(t2, "typeof C2");
AssertType(C2.bar(), "typeof C2");
AssertType(C2.bar, "() => typeof C2");

// all ok
let r5 = t2.foo + 1;
AssertType(r5, "string");
AssertType(t2.foo + 1, "string");
AssertType(t2.foo, "string");
AssertType(1, "int");

let r6 = t2.bar();
AssertType(r6, "typeof C2");
AssertType(t2.bar(), "typeof C2");
AssertType(t2.bar, "() => typeof C2");

let r7 = new t2('');
AssertType(r7, "C2<unknown>");
AssertType(new t2(''), "C2<unknown>");
AssertType(t2, "typeof C2");
AssertType('', "string");



