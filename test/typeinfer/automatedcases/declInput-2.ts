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

// === tests/cases/compiler/declInput-2.ts ===
declare function AssertType(value:any, type:string):void;
module M {
    class C { }
    export class E {}
    export interface I1 {}
    interface I2 {}
    export class D {
        private c: C; // don't generate
        public m1: number;
        public m2: string;
        public m22: C; // don't generate
        public m23: E;
        public m24: I1;
        public m25: I2; // don't generate
        public m232(): E { 
AssertType(null, "null");
return null;
}

        public m242(): I1 { 
AssertType(null, "null");
return null; 
}

        public m252(): I2 { 
AssertType(null, "null");
return null; } // don't generate

        public m26(i:I1) {}
        public m262(i:I2) {}
        public m3():C { 
AssertType(new C(), "C");
return new C(); 

AssertType(C, "typeof C");
}
    }
}

