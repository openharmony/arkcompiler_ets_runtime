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

// === tests/cases/compiler/exportPrivateType.ts ===
declare function AssertType(value:any, type:string):void;
module foo {
    class C1 {
        x: string;
        y: C1;
    }
 
    class C2 {
        test() { 
AssertType(true, "boolean");
return true; 
}
    }
 
    interface I1 {
        (a: string, b: string): string;
        (x: number, y: number): I1;
    }
 
    interface I2 {
        x: string;
        y: number;
    }
 
    // None of the types are exported, so per section 10.3, should all be errors
    export let e: C1;
    export let f: I1;
    export let g: C2;
    export let h: I2;
}
 
let y = foo.g; // Exported letiable 'y' has or is using private type 'foo.C2'.
AssertType(y, "C2");
AssertType(foo.g, "C2");



