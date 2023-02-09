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

// === tests/cases/compiler/index.ts ===
declare function AssertType(value:any, type:string):void;
import {x} from './x.js'

const x2 = {x
AssertType(x2, "{ x: string; }");

AssertType({x}, "{ x: string; }");

AssertType(x, "string");
}

const a = {x2
AssertType(a, "{ x2: { x: string; }; }");

AssertType({x2}, "{ x2: { x: string; }; }");

AssertType(x2, "{ x: string; }");
}

const x3 = x
AssertType(x3, "string");
AssertType(x, "string");

const b = {x3
AssertType(b, "{ x3: string; }");

AssertType({x3}, "{ x3: string; }");

AssertType(x3, "string");
}


