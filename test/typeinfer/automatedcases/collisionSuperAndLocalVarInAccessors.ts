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

// === tests/cases/compiler/collisionSuperAndLocalVarInAccessors.ts ===
declare function AssertType(value:any, type:string):void;
let _super = 10; // No Error
AssertType(_super, "number");
AssertType(10, "int");

class Foo {
    get prop1(): number {
        let _super = 10; // No error
AssertType(_super, "number");
AssertType(10, "int");

AssertType(10, "int");
        return 10;
    }
    set prop1(val: number) {
        let _super = 10; // No error
AssertType(_super, "number");
AssertType(10, "int");
    }
}
class b extends Foo {
    get prop2(): number {
        let _super = 10; // Should be error
AssertType(_super, "number");
AssertType(10, "int");

AssertType(10, "int");
        return 10;
    }
    set prop2(val: number) {
        let _super = 10; // Should be error
AssertType(_super, "number");
AssertType(10, "int");
    }
}
class c extends Foo {
    get prop2(): number {
        let x = () => {
AssertType(x, "() => void");
AssertType(() => {            let _super = 10; // Should be error        }, "() => void");

            let _super = 10; // Should be error
AssertType(_super, "number");
AssertType(10, "int");
        }
AssertType(10, "int");
        return 10;
    }
    set prop2(val: number) {
        let x = () => {
AssertType(x, "() => void");
AssertType(() => {            let _super = 10; // Should be error        }, "() => void");

            let _super = 10; // Should be error
AssertType(_super, "number");
AssertType(10, "int");
        }
    }
}

