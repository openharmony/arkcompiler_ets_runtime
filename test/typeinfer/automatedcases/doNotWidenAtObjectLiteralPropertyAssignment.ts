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

// === tests/cases/compiler/doNotWidenAtObjectLiteralPropertyAssignment.ts ===
declare function AssertType(value:any, type:string):void;
interface ITestEventInterval {
    begin: number;
}

interface IIntervalTreeNode {
    interval: ITestEventInterval;
    children?: IIntervalTreeNode[];
}

let test: IIntervalTreeNode[] = [{ interval: { begin: 0 }, children: null }]; // was error here because best common type is {
AssertType(test, "IIntervalTreeNode[]");

AssertType([{ interval: { begin: 0 }, children: null }], "{ interval: { begin: number; }; children: null; }[]");

AssertType({ interval: { begin: 0 }, children: null }, "{ interval: { begin: number; }; children: null; }");

AssertType(interval, "{ begin: number; }");

AssertType({ begin: 0 }, "{ begin: number; }");

AssertType(begin, "number");

AssertType(0, "int");

AssertType(children, "null");

AssertType(null, "null");
}


