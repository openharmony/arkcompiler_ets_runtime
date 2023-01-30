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

// === tests/cases/compiler/collisionRestParameterClassMethod.ts ===
declare function AssertType(value:any, type:string):void;
class c1 {
    public foo(_i: number, ...restParameters) { //_i is error
        let _i = 10; // no error
AssertType(_i, "number");
AssertType(10, "int");
    }
    public fooNoError(_i: number) { // no error
        let _i = 10; // no error
AssertType(_i, "number");
AssertType(10, "int");
    }
    public f4(_i: number, ...rest); // no codegen no error
    public f4(_i: string, ...rest); // no codegen no error
    public f4(_i: any, ...rest) { // error
        let _i: any; // no error
AssertType(_i, "any");
    }

    public f4NoError(_i: number); // no error
    public f4NoError(_i: string); // no error
    public f4NoError(_i: any) { // no error
        let _i: any; // no error
AssertType(_i, "any");
    }
}

declare class c2 {
    public foo(_i: number, ...restParameters); // No error - no code gen
    public fooNoError(_i: number); // no error

    public f4(_i: number, ...rest); // no codegen no error
    public f4(_i: string, ...rest); // no codegen no error
    public f4NoError(_i: number); // no error
    public f4NoError(_i: string); // no error
}

class c3 {
    public foo(...restParameters) {
        let _i = 10; // no error
AssertType(_i, "number");
AssertType(10, "int");
    }
    public fooNoError() {
        let _i = 10; // no error
AssertType(_i, "number");
AssertType(10, "int");
    }
}

