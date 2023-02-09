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

// === c:/root/folder1/file1.ts ===
declare function AssertType(value:any, type:string):void;
import {x} from "folder2/file1"
import {y} from "folder3/file2"
import {z} from "components/file3"
import {z1} from "file4"

declare function use(a: any): void;

use(x.toExponential());
AssertType(use(x.toExponential()), "void");
AssertType(use, "(any) => void");
AssertType(x.toExponential(), "string");
AssertType(x.toExponential, "(?number) => string");

use(y.toExponential());
AssertType(use(y.toExponential()), "void");
AssertType(use, "(any) => void");
AssertType(y.toExponential(), "string");
AssertType(y.toExponential, "(?number) => string");

use(z.toExponential());
AssertType(use(z.toExponential()), "void");
AssertType(use, "(any) => void");
AssertType(z.toExponential(), "string");
AssertType(z.toExponential, "(?number) => string");

use(z1.toExponential());
AssertType(use(z1.toExponential()), "void");
AssertType(use, "(any) => void");
AssertType(z1.toExponential(), "string");
AssertType(z1.toExponential, "(?number) => string");


// === c:/root/folder2/file1.ts ===
declare function AssertType(value:any, type:string):void;
export let x = 1;
AssertType(x, "number");
AssertType(1, "int");


// === c:/root/generated/folder3/file2.ts ===
declare function AssertType(value:any, type:string):void;
export let y = 1;
AssertType(y, "number");
AssertType(1, "int");


// === c:/root/shared/components/file3.ts ===
declare function AssertType(value:any, type:string):void;
export let z = 1;
AssertType(z, "number");
AssertType(1, "int");


// === c:/file4.ts ===
declare function AssertType(value:any, type:string):void;
export let z1 = 1;
AssertType(z1, "number");
AssertType(1, "int");


