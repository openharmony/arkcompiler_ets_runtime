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

// === tests/cases/compiler/classImplementsClass3.ts ===
declare function AssertType(value:any, type:string):void;
class A { foo(): number { 
AssertType(1, "int");
return 1; } 
}

class C implements A {
    foo() {
AssertType(1, "int");
        return 1;
    }
}

class C2 extends A {}

// no errors
let c: C;
AssertType(c, "C");

let c2: C2;
AssertType(c2, "C2");

c = c2;
AssertType(c = c2, "C2");
AssertType(c, "C");
AssertType(c2, "C2");

c2 = c;
AssertType(c2 = c, "C");
AssertType(c2, "C2");
AssertType(c, "C");


