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

// === /a.ts ===
declare function AssertType(value:any, type:string):void;
import foo from "foo";
foo.bar();
AssertType(foo.bar(), "number");
AssertType(foo.bar, "() => number");


// === /node_modules/foo/index.js ===
declare function AssertType(value:any, type:string):void;
// Same as untypedModuleImport.ts but with --allowJs, so the package will actually be typed.

exports.default = { bar() { 
AssertType(exports.default = { bar() { return 0; } }, "{ bar(): number; }");
return 0; } 

AssertType(exports.default, "{ bar(): number; }");

AssertType({ bar() { return 0; } }, "{ bar(): number; }");

AssertType(bar, "() => number");

AssertType(0, "int");
}


