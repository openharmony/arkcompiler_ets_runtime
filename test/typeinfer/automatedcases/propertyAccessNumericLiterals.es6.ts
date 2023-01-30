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

// === tests/cases/conformance/es6/propertyAccess/propertyAccessNumericLiterals.es6.ts ===
declare function AssertType(value:any, type:string):void;
0xffffffff.toString();
AssertType(0xffffffff.toString(), "string");
AssertType(0xffffffff.toString, "(?number) => string");

0o01234.toString();
AssertType(0o01234.toString(), "string");
AssertType(0o01234.toString, "(?number) => string");

0b01101101.toString();
AssertType(0b01101101.toString(), "string");
AssertType(0b01101101.toString, "(?number) => string");

1234..toString();
AssertType(1234..toString(), "string");
AssertType(1234..toString, "(?number) => string");

1e0.toString();
AssertType(1e0.toString(), "string");
AssertType(1e0.toString, "(?number) => string");


