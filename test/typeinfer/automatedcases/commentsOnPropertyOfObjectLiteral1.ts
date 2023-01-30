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

// === tests/cases/compiler/commentsOnPropertyOfObjectLiteral1.ts ===
declare function AssertType(value:any, type:string):void;
let resolve = {
AssertType(resolve, "{ id: (any) => any; id1: string; id2: (any) => any; id3: (any) => any; id4: (any) => any; }");
AssertType({    id: /*! @ngInject */ (details: any) => details.id,    id1: /* c1 */ "hello",    id2:        /*! @ngInject */ (details: any) => details.id,    id3:    /*! @ngInject */    (details: any) => details.id,    id4:    /*! @ngInject */    /* C2 */    (details: any) => details.id,}, "{ id: (any) => any; id1: string; id2: (any) => any; id3: (any) => any; id4: (any) => any; }");

    id: /*! @ngInject */ (details: any) => details.id,
AssertType(id, "(any) => any");
AssertType((details: any) => details.id, "(any) => any");
AssertType(details, "any");
AssertType(details.id, "any");

    id1: /* c1 */ "hello",
AssertType(id1, "string");
AssertType("hello", "string");

    id2:
AssertType(id2, "(any) => any");

        /*! @ngInject */ (details: any) => details.id,
AssertType((details: any) => details.id, "(any) => any");
AssertType(details, "any");
AssertType(details.id, "any");

    id3:
AssertType(id3, "(any) => any");

    /*! @ngInject */
    (details: any) => details.id,
AssertType((details: any) => details.id, "(any) => any");
AssertType(details, "any");
AssertType(details.id, "any");

    id4:
AssertType(id4, "(any) => any");

    /*! @ngInject */
    /* C2 */
    (details: any) => details.id,
AssertType((details: any) => details.id, "(any) => any");
AssertType(details, "any");
AssertType(details.id, "any");

};

