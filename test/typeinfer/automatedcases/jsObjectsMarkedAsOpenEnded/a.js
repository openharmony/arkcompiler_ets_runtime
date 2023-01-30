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

// === tests/cases/conformance/salsa/a.js ===
declare function AssertType(value:any, type:string):void;
let letiable = {};
AssertType(letiable, "typeof letiable");
AssertType({}, "{}");

letiable.a = 0;
AssertType(letiable.a = 0, "int");
AssertType(letiable.a, "number");
AssertType(0, "int");

class C {
    initializedMember = {};
    constructor() {
        this.member = {};
AssertType(this.member = {}, "{}");
AssertType(this.member, "any");
AssertType(this, "this");
AssertType({}, "{}");

        this.member.a = 0;
AssertType(this.member.a = 0, "int");
AssertType(this.member.a, "error");
AssertType(this.member, "{}");
AssertType(this, "this");
AssertType(0, "int");
    }
}

let obj = {
AssertType(obj, "{ property: {}; }");
AssertType({    property: {}}, "{ property: {}; }");

    property: {
AssertType(property, "{}");

AssertType({}, "{}");
}

};

obj.property.a = 0;
AssertType(obj.property.a = 0, "int");
AssertType(obj.property.a, "error");
AssertType(0, "int");

let arr = [{}];
AssertType(arr, "{}[]");
AssertType([{}], "{}[]");
AssertType({}, "{}");

function getObj() {
AssertType({}, "{}");
    return {};
}



