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

// === tests/cases/compiler/commentsMultiModuleMultiFile_0.ts ===
declare function AssertType(value:any, type:string):void;
/** this is multi declare module*/
export module multiM {
    /// class b comment
    export class b {
    }
}
/** thi is multi module 2*/
export module multiM {
    /** class c comment*/
    export class c {
    }

    // class e comment
    export class e {
    }
}

new multiM.b();
AssertType(new multiM.b(), "multiM.b");
AssertType(multiM.b, "typeof multiM.b");

new multiM.c();
AssertType(new multiM.c(), "multiM.c");
AssertType(multiM.c, "typeof multiM.c");


