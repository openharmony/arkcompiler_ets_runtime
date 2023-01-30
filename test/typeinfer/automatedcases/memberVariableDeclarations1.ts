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

// === tests/cases/compiler/memberVariableDeclarations1.ts ===
declare function AssertType(value:any, type:string):void;
// from spec

class Employee {
    public name: string;
    public address: string;
    public retired = false;
    public manager: Employee = null;
    public reports: Employee[] = [];
}

class Employee2 {
    public name: string;
    public address: string;
    public retired: boolean;
    public manager: Employee;
    public reports: Employee[];
    constructor() {
        this.retired = false;
AssertType(this.retired = false, "boolean");
AssertType(this.retired, "boolean");
AssertType(this, "this");
AssertType(false, "boolean");

        this.manager = null;
AssertType(this.manager = null, "null");
AssertType(this.manager, "Employee");
AssertType(this, "this");
AssertType(null, "null");

        this.reports = [];
AssertType(this.reports = [], "undefined[]");
AssertType(this.reports, "Employee[]");
AssertType(this, "this");
AssertType([], "undefined[]");
    }
}

let e1: Employee;
AssertType(e1, "Employee");

let e2: Employee2;
AssertType(e2, "Employee2");

e1 = e2;
AssertType(e1 = e2, "Employee2");
AssertType(e1, "Employee");
AssertType(e2, "Employee2");

e2 = e1;
AssertType(e2 = e1, "Employee");
AssertType(e2, "Employee2");
AssertType(e1, "Employee");


