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

// === tests/cases/compiler/switchCaseInternalComments.ts ===
declare function AssertType(value:any, type:string):void;

/*-1*/ foo /*0*/ : /*1*/ switch /*2*/ ( /*3*/ false /*4*/ ) /*5*/ {
    /*6*/ case /*7*/ false /*8*/ : /*9*/
        /*10*/ break /*11*/ foo /*12*/;
    /*13*/ default /*14*/ : /*15*/
    /*16*/ case /*17*/ false /*18*/ : /*19*/ { /*20*/
    /*21*/ } /*22*/
}

