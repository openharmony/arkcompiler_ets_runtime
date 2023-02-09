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

// === tests/cases/compiler/declFileModuleAssignmentInObjectLiteralProperty.ts ===
declare function AssertType(value:any, type:string):void;
module m1 {
    export class c {
    }
}
let d = {
AssertType(d, "{ m1: { m: typeof m1; }; m2: { c: typeof m1.c; }; }");
AssertType({    m1: { m: m1 },    m2: { c: m1.c },}, "{ m1: { m: typeof m1; }; m2: { c: typeof m1.c; }; }");

    m1: { m: m1 },
AssertType(m1, "{ m: typeof m1; }");
AssertType({ m: m1 }, "{ m: typeof m1; }");
AssertType(m, "typeof m1");
AssertType(m1, "typeof m1");

    m2: { c: m1.c },
AssertType(m2, "{ c: typeof m1.c; }");
AssertType({ c: m1.c }, "{ c: typeof m1.c; }");
AssertType(c, "typeof m1.c");
AssertType(m1.c, "typeof m1.c");

};

