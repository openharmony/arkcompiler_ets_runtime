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

// === tests/cases/conformance/es6/templates/templateStringInEqualityChecks.ts ===
declare function AssertType(value:any, type:string):void;
let x = `abc${0}abc` === `abc` ||
AssertType(x, "boolean");
AssertType(`abc${0}abc` === `abc` ||        `abc` !== `abc${0}abc` &&        `abc${0}abc` == "abc0abc" &&        "abc0abc" !== `abc${0}abc`, "boolean");
AssertType(`abc${0}abc` === `abc`, "boolean");
AssertType(`abc${0}abc`, "string");
AssertType(0, "int");
AssertType(`abc`, "string");

        `abc` !== `abc${0}abc` &&
AssertType(`abc` !== `abc${0}abc` &&        `abc${0}abc` == "abc0abc" &&        "abc0abc" !== `abc${0}abc`, "boolean");
AssertType(`abc` !== `abc${0}abc` &&        `abc${0}abc` == "abc0abc", "boolean");
AssertType(`abc` !== `abc${0}abc`, "boolean");
AssertType(`abc`, "string");
AssertType(`abc${0}abc`, "string");
AssertType(0, "int");

        `abc${0}abc` == "abc0abc" &&
AssertType(`abc${0}abc` == "abc0abc", "boolean");
AssertType(`abc${0}abc`, "string");
AssertType(0, "int");
AssertType("abc0abc", "string");

        "abc0abc" !== `abc${0}abc`;
AssertType("abc0abc" !== `abc${0}abc`, "boolean");
AssertType("abc0abc", "string");
AssertType(`abc${0}abc`, "string");
AssertType(0, "int");


