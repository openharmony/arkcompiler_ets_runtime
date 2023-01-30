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

// === tests/cases/compiler/file1.ts ===
declare function AssertType(value:any, type:string):void;
import * as moduleB from "./file2"

declare function use(v: any): void;

use(moduleB.value);
AssertType(use(moduleB.value), "void");
AssertType(use, "(any) => void");
AssertType(moduleB.value, "string");

use(moduleB.moduleC);
AssertType(use(moduleB.moduleC), "void");
AssertType(use, "(any) => void");
AssertType(moduleB.moduleC, "string");

use(moduleB.moduleCStar);
AssertType(use(moduleB.moduleCStar), "void");
AssertType(use, "(any) => void");
AssertType(moduleB.moduleCStar, "typeof moduleB.moduleCStar");


