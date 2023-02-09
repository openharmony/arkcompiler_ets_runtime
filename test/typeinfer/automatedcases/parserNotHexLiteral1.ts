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

// === tests/cases/conformance/parser/ecmascript5/RegressionTests/parserNotHexLiteral1.ts ===
declare function AssertType(value:any, type:string):void;
let x = {e0: 'cat', x0: 'dog'};
AssertType(x, "{ e0: string; x0: string; }");
AssertType({e0: 'cat', x0: 'dog'}, "{ e0: string; x0: string; }");
AssertType(e0, "string");
AssertType('cat', "string");
AssertType(x0, "string");
AssertType('dog', "string");

console.info (x.x0);
AssertType(console.info (x.x0), "void");
AssertType(console.info, "(...any[]) => void");
AssertType(x.x0, "string");

// tsc dies on this next line with "bug.ts (5,16): Expected ')'"
//   tsc seems to be parsing the e0 as a hex constant.
console.info (x.e0);
AssertType(console.info (x.e0), "void");
AssertType(console.info, "(...any[]) => void");
AssertType(x.e0, "string");


