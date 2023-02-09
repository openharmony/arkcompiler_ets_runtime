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

// === tests/cases/compiler/forStatementInnerComments.ts ===
declare function AssertType(value:any, type:string):void;
declare let a;
AssertType(a, "any");

/*0*/ for /*1*/ ( /*2*/ let /*3*/ x /*4*/ in /*5*/ a /*6*/) /*7*/ {}
/*0*/ for /*1*/ ( /*2*/ let /*3*/ y /*4*/ of /*5*/ a /*6*/) /*7*/ {}
/*0*/ for /*1*/ ( /*2*/ x /*3*/ in /*4*/ a /*5*/) /*6*/ {}
/*0*/ for /*1*/ ( /*2*/ y /*3*/ of /*4*/ a /*5*/) /*6*/ {}
/*0*/ for /*1*/ ( /*2*/ a /*3*/ ; /*4*/ a  /*5*/ ; /*6*/ a /*7*/) /*8*/ {}


