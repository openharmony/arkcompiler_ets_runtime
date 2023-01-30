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

// === tests/cases/compiler/declareFileExportAssignmentWithVarFromVariableStatement.ts ===
declare function AssertType(value:any, type:string):void;
module m2 {
    export interface connectModule {
        (res, req, next): void;
    }
    export interface connectExport {
        use: (mod: connectModule) => connectExport;
        listen: (port: number) => void;
    }

}

let x = 10, m2: {
AssertType(x, "number");
AssertType(10, "int");
AssertType(m2, "{ (): m2.connectExport; test1: m2.connectModule; test2(): m2.connectModule; }");

    (): m2.connectExport;
AssertType(m2, "any");

    test1: m2.connectModule;
AssertType(test1, "m2.connectModule");
AssertType(m2, "any");

    test2(): m2.connectModule;
AssertType(test2, "() => m2.connectModule");
AssertType(m2, "any");

};

export default m2;

