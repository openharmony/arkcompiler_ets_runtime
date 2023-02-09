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

// === tests/cases/conformance/classes/classDeclarations/file1.ts ===
declare function AssertType(value:any, type:string):void;
declare class C1 { }

interface C1 { }

interface C2 { }

declare class C2 { }

class C3 { }

interface C3 { }

interface C4 { }

class C4 { }

interface C5 {
    x1: number;
}

declare class C5 {
    x2: number;
}

interface C5 {
    x3: number;
}

interface C5 {
    x4: number;
}

// checks if properties actually were merged
let c5 : C5;
AssertType(c5, "C5");

c5.x1;
AssertType(c5.x1, "number");

c5.x2;
AssertType(c5.x2, "number");

c5.x3;
AssertType(c5.x3, "number");

c5.x4;
AssertType(c5.x4, "number");


