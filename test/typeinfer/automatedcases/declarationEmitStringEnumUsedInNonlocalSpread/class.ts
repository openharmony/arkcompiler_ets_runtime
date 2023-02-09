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

// === tests/cases/compiler/class.ts ===
declare function AssertType(value:any, type:string):void;
export const enum TestEnum {
    Test1 = '123123',
    Test2 = '12312312312',
}

export interface ITest {
    [TestEnum.Test1]: string;
    [TestEnum.Test2]: string;
}

export class A {
    getA(): ITest {
AssertType({            [TestEnum.Test1]: '123',            [TestEnum.Test2]: '123',        }, "{ 123123: string; 12312312312: string; }");
        return {

            [TestEnum.Test1]: '123',
AssertType([TestEnum.Test1], "string");
AssertType(TestEnum.Test1, "TestEnum.Test1");
AssertType('123', "string");

            [TestEnum.Test2]: '123',
AssertType([TestEnum.Test2], "string");
AssertType(TestEnum.Test2, "TestEnum.Test2");
AssertType('123', "string");

        };
    }
}

