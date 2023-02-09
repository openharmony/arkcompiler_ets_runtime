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

// === tests/cases/compiler/commentsExternalModules_1.ts ===
declare function AssertType(value:any, type:string):void;
/**This is on import declaration*/
import * as extMod from "./commentsExternalModules2_0"; // trailing comment 1
extMod.m1.fooExport();
AssertType(extMod.m1.fooExport(), "number");
AssertType(extMod.m1.fooExport, "() => number");

export let newVar = new extMod.m1.m2.c();
AssertType(newVar, "extMod.m1.m2.c");
AssertType(new extMod.m1.m2.c(), "extMod.m1.m2.c");
AssertType(extMod.m1.m2.c, "typeof extMod.m1.m2.c");

extMod.m4.fooExport();
AssertType(extMod.m4.fooExport(), "number");
AssertType(extMod.m4.fooExport, "() => number");

export let newVar2 = new extMod.m4.m2.c();
AssertType(newVar2, "extMod.m4.m2.c");
AssertType(new extMod.m4.m2.c(), "extMod.m4.m2.c");
AssertType(extMod.m4.m2.c, "typeof extMod.m4.m2.c");


