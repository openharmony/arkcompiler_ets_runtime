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

// === tests/cases/conformance/es6/templates/taggedTemplateStringsWithTypedTags.ts ===
declare function AssertType(value:any, type:string):void;
interface I {
    (stringParts: TemplateStringsArray, ...rest: number[]): I;
    g: I;
    h: I;
    member: I;
    thisIsNotATag(x: string): void
    [x: number]: I;
}

let f: I;
AssertType(f, "I");

f `abc`
AssertType(f `abc`, "I");
AssertType(f, "I");
AssertType(`abc`, "string");

f `abc${1}def${2}ghi`;
AssertType(f `abc${1}def${2}ghi`, "I");
AssertType(f, "I");
AssertType(`abc${1}def${2}ghi`, "string");
AssertType(1, "int");
AssertType(2, "int");

f `abc`.member
AssertType(f `abc`.member, "I");

f `abc${1}def${2}ghi`.member;
AssertType(f `abc${1}def${2}ghi`.member, "I");

f `abc`["member"];
AssertType(f `abc`["member"], "I");
AssertType(f `abc`, "I");
AssertType(f, "I");
AssertType(`abc`, "string");
AssertType("member", "string");

f `abc${1}def${2}ghi`["member"];
AssertType(f `abc${1}def${2}ghi`["member"], "I");
AssertType(f `abc${1}def${2}ghi`, "I");
AssertType(f, "I");
AssertType(`abc${1}def${2}ghi`, "string");
AssertType(1, "int");
AssertType(2, "int");
AssertType("member", "string");

f `abc`[0].member `abc${1}def${2}ghi`;
AssertType(f `abc`[0].member `abc${1}def${2}ghi`, "I");
AssertType(f `abc`[0].member, "I");
AssertType(`abc${1}def${2}ghi`, "string");
AssertType(1, "int");
AssertType(2, "int");

f `abc${1}def${2}ghi`["member"].member `abc${1}def${2}ghi`;
AssertType(f `abc${1}def${2}ghi`["member"].member `abc${1}def${2}ghi`, "I");
AssertType(f `abc${1}def${2}ghi`["member"].member, "I");
AssertType(`abc${1}def${2}ghi`, "string");
AssertType(1, "int");
AssertType(2, "int");

f.thisIsNotATag(`abc`);
AssertType(f.thisIsNotATag(`abc`), "void");
AssertType(f.thisIsNotATag, "(string) => void");
AssertType(`abc`, "string");

f.thisIsNotATag(`abc${1}def${2}ghi`);
AssertType(f.thisIsNotATag(`abc${1}def${2}ghi`), "void");
AssertType(f.thisIsNotATag, "(string) => void");
AssertType(`abc${1}def${2}ghi`, "string");
AssertType(1, "int");
AssertType(2, "int");


