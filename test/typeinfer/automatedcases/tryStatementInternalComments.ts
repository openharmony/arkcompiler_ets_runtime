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

// === tests/cases/compiler/tryStatementInternalComments.ts ===
declare function AssertType(value:any, type:string):void;
/*1*/ try /*2*/ { /*3*/
    /*4*/ throw /*5*/ "no" /*6*/;
AssertType("no", "string");

/*7*/} /*8*/ catch /*9*/ ( /*10*/ e /*11*/ ) /*12*/ { /*13*/

/*14*/} /*15*/ finally /*16*/ { /*17*/

/*18*/} /*19*/

