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

// === tests/cases/conformance/es6/templates/taggedTemplateStringsWithTagsTypedAsAnyES6.ts ===
declare function AssertType(value:any, type:string):void;
let f: any;
AssertType(f, "any");

f `abc`
AssertType(f `abc`, "any");
AssertType(f, "any");
AssertType(`abc`, "string");

f `abc${1}def${2}ghi`;
AssertType(f `abc${1}def${2}ghi`, "any");
AssertType(f, "any");
AssertType(`abc${1}def${2}ghi`, "string");
AssertType(1, "int");
AssertType(2, "int");

f.g.h `abc`
AssertType(f.g.h `abc`, "any");
AssertType(f.g.h, "any");
AssertType(`abc`, "string");

f.g.h `abc${1}def${2}ghi`;
AssertType(f.g.h `abc${1}def${2}ghi`, "any");
AssertType(f.g.h, "any");
AssertType(`abc${1}def${2}ghi`, "string");
AssertType(1, "int");
AssertType(2, "int");

f `abc`.member
AssertType(f `abc`.member, "any");

f `abc${1}def${2}ghi`.member;
AssertType(f `abc${1}def${2}ghi`.member, "any");

f `abc`["member"];
AssertType(f `abc`["member"], "any");
AssertType(f `abc`, "any");
AssertType(f, "any");
AssertType(`abc`, "string");
AssertType("member", "string");

f `abc${1}def${2}ghi`["member"];
AssertType(f `abc${1}def${2}ghi`["member"], "any");
AssertType(f `abc${1}def${2}ghi`, "any");
AssertType(f, "any");
AssertType(`abc${1}def${2}ghi`, "string");
AssertType(1, "int");
AssertType(2, "int");
AssertType("member", "string");

f `abc`["member"].someOtherTag `abc${1}def${2}ghi`;
AssertType(f `abc`["member"].someOtherTag `abc${1}def${2}ghi`, "any");
AssertType(f `abc`["member"].someOtherTag, "any");
AssertType(`abc${1}def${2}ghi`, "string");
AssertType(1, "int");
AssertType(2, "int");

f `abc${1}def${2}ghi`["member"].someOtherTag `abc${1}def${2}ghi`;
AssertType(f `abc${1}def${2}ghi`["member"].someOtherTag `abc${1}def${2}ghi`, "any");
AssertType(f `abc${1}def${2}ghi`["member"].someOtherTag, "any");
AssertType(`abc${1}def${2}ghi`, "string");
AssertType(1, "int");
AssertType(2, "int");

f.thisIsNotATag(`abc`);
AssertType(f.thisIsNotATag(`abc`), "any");
AssertType(f.thisIsNotATag, "any");
AssertType(`abc`, "string");

f.thisIsNotATag(`abc${1}def${2}ghi`);
AssertType(f.thisIsNotATag(`abc${1}def${2}ghi`), "any");
AssertType(f.thisIsNotATag, "any");
AssertType(`abc${1}def${2}ghi`, "string");
AssertType(1, "int");
AssertType(2, "int");


