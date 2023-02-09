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

// === tests/cases/compiler/declarationEmitClassMemberNameConflict.ts ===
declare function AssertType(value:any, type:string):void;
export class C1 {
    C1() { } // has to be the same as the class name

    bar() {
AssertType(function (t: typeof C1) {        }, "(typeof C1) => void");
AssertType(t, "typeof C1");
AssertType(C1, "typeof C1");
        return function (t: typeof C1) {

        };
    }
}

export class C2 {
    C2: any // has to be the same as the class name

    bar() {
AssertType(function (t: typeof C2) {        }, "(typeof C2) => void");
AssertType(t, "typeof C2");
AssertType(C2, "typeof C2");
        return function (t: typeof C2) {

        };
    }
}

export class C3 {
    get C3() { 
AssertType(0, "int");
return 0; } // has to be the same as the class name

    bar() {
AssertType(function (t: typeof C3) {        }, "(typeof C3) => void");
AssertType(t, "typeof C3");
AssertType(C3, "typeof C3");
        return function (t: typeof C3) {

        };
    }
}

export class C4 {
    set C4(v) { } // has to be the same as the class name

    bar() {
AssertType(function (t: typeof C4) {        }, "(typeof C4) => void");
AssertType(t, "typeof C4");
AssertType(C4, "typeof C4");
        return function (t: typeof C4) {

        };
    }
}

