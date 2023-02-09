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

// === tests/cases/compiler/exhaustiveSwitchWithWideningLiteralTypes.ts ===
declare function AssertType(value:any, type:string):void;
// Repro from #12529

class A {
    readonly kind = "A"; // (property) A.kind: "A"
}

class B {
    readonly kind = "B"; // (property) B.kind: "B"
}

function f(value: A | B): number {
    switch(value.kind) {
AssertType(value.kind, "union");

        case "A": 
AssertType("A", "string");
AssertType(0, "int");
return 0;

        case "B": 
AssertType("B", "string");
AssertType(1, "int");
return 1;
    }
}

