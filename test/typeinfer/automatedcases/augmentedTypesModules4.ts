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

// === tests/cases/compiler/augmentedTypesModules4.ts ===
declare function AssertType(value:any, type:string):void;

// module then enum
// should be errors
module m4 { }
enum m4 { }

module m4a { let y = 2; }
enum m4a { One }

module m4b { export let y = 2; }
enum m4b { One }

module m4c { interface I { foo(): void } }
enum m4c { One }

module m4d { class C { foo() { } } }
enum m4d { One }

//// module then module

module m5 { export let y = 2; }
module m5 { export interface I { foo(): void } } // should already be reasonably well covered


