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

// === tests/cases/conformance/jsdoc/declarations/index.js ===
declare function AssertType(value:any, type:string):void;
const j = require("./package.json");
AssertType(j, "{ name: string; version: string; description: string; main: string; bin: { cli: string; }; engines: { node: string; }; scripts: { scriptname: string; }; devDependencies: { "@ns/dep": string; }; dependencies: { dep: string; }; repository: string; keywords: string[]; author: string; license: string; homepage: string; config: { o: string[]; }; }");
AssertType(require("./package.json"), "{ name: string; version: string; description: string; main: string; bin: { cli: string; }; engines: { node: string; }; scripts: { scriptname: string; }; devDependencies: { "@ns/dep": string; }; dependencies: { dep: string; }; repository: string; keywords: string[]; author: string; license: string; homepage: string; config: { o: string[]; }; }");
AssertType(require, "any");
AssertType("./package.json", "string");

module.exports = j;
AssertType(module.exports = j, "{ name: string; version: string; description: string; main: string; bin: { cli: string; }; engines: { node: string; }; scripts: { scriptname: string; }; devDependencies: { "@ns/dep": string; }; dependencies: { dep: string; }; repository: string; keywords: string[]; author: string; license: string; homepage: string; config: { o: string[]; }; }");
AssertType(module.exports, "{ name: string; version: string; description: string; main: string; bin: { cli: string; }; engines: { node: string; }; scripts: { scriptname: string; }; devDependencies: { "@ns/dep": string; }; dependencies: { dep: string; }; repository: string; keywords: string[]; author: string; license: string; homepage: string; config: { o: string[]; }; }");
AssertType(j, "{ name: string; version: string; description: string; main: string; bin: { cli: string; }; engines: { node: string; }; scripts: { scriptname: string; }; devDependencies: { "@ns/dep": string; }; dependencies: { dep: string; }; repository: string; keywords: string[]; author: string; license: string; homepage: string; config: { o: string[]; }; }");


