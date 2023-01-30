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

// === tests/cases/compiler/es5-asyncFunctionReturnStatements.ts ===
declare function AssertType(value:any, type:string):void;
declare let x, y, z, a, b, c;
AssertType(x, "any");
AssertType(y, "any");
AssertType(z, "any");
AssertType(a, "any");
AssertType(b, "any");
AssertType(c, "any");

async function 
returnStatement0(): Promise<any> {
    return;
}

async function 
returnStatement1(): Promise<any> {
AssertType(x, "any");
    return x;
}

async function 
returnStatement2(): Promise<any> {
AssertType(await x, "any");
AssertType(x, "any");
    return await x;
}

async function 
returnStatement3(): Promise<any> {
    { 
return; }
}

async function 
returnStatement4(): Promise<any> {
    await x;
AssertType(await x, "any");
AssertType(x, "any");

    { 
return; }
}

async function 
returnStatement5(): Promise<any>{
    { 
AssertType(await x, "any");
return await x; 

AssertType(x, "any");
}
}

