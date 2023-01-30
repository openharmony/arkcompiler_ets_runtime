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

// === tests/cases/compiler/es6ImportDefaultBinding_1.ts ===
declare function AssertType(value:any, type:string):void;
import defaultBinding from "es6ImportDefaultBinding_0";
let x = defaultBinding;
AssertType(x, "number");
AssertType(defaultBinding, "number");

import defaultBinding2 from "es6ImportDefaultBinding_0"; // elide this import since defaultBinding2 is not used


