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

// === tests/cases/conformance/classes/constructorDeclarations/superCalls/emitStatementsBeforeSuperCallWithDefineFields.ts ===
declare function AssertType(value:any, type:string):void;
class Base {
}
class Sub extends Base {
    // @ts-ignore
    constructor(public p: number) {
        console.log('hi');
AssertType(console.log('hi'), "void");
AssertType(console.log, "(...any[]) => void");
AssertType('hi', "string");

        super();
AssertType(super(), "void");
AssertType(super, "typeof Base");
    }
    field = 0;
}

class Test extends Base {
    prop: number;
    // @ts-ignore
    constructor(public p: number) {
        1;
AssertType(1, "int");

        super();
AssertType(super(), "void");
AssertType(super, "typeof Base");

        this.prop = 1;
AssertType(this.prop = 1, "int");
AssertType(this.prop, "number");
AssertType(this, "this");
AssertType(1, "int");
    }
}


