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

// === tests/cases/compiler/augmentedTypesModules3b.ts ===
declare function AssertType(value:any, type:string):void;

class m3b { foo() { } }
module m3b { let y = 2; }

class m3c { foo() { } }
module m3c { export let y = 2; } 

declare class m3d { foo(): void }
module m3d { export let y = 2; } 

module m3e { export let y = 2; } 
declare class m3e { foo(): void } 

declare class m3f { foo(): void }
module m3f { export interface I { foo(): void } }

declare class m3g { foo(): void }
module m3g { export class C { foo() { } } }


