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

// === tests/cases/compiler/privacyFunc.ts ===
declare function AssertType(value:any, type:string):void;
module m1 {
    export class C1_public {
        private f1() {
        }
    }

    class C2_private {
    }

    export class C3_public {
        constructor (m1_c3_c1: C1_public);
        constructor (m1_c3_c2: C2_private); //error
        constructor (m1_c3_c1_2: any) {
        }

        private f1_private(m1_c3_f1_arg: C1_public) {
        }

        public f2_public(m1_c3_f2_arg: C1_public) {
        }

        private f3_private(m1_c3_f3_arg: C2_private) {
        }

        public f4_public(m1_c3_f4_arg: C2_private) { // error
        }

        private f5_private() {
AssertType(new C1_public(), "C1_public");
AssertType(C1_public, "typeof C1_public");
            return new C1_public();
        }

        public f6_public() {
AssertType(new C1_public(), "C1_public");
AssertType(C1_public, "typeof C1_public");
            return new C1_public();
        }

        private f7_private() {
AssertType(new C2_private(), "C2_private");
AssertType(C2_private, "typeof C2_private");
            return new C2_private();
        }

        public f8_public() {
AssertType(new C2_private(), "C2_private");
AssertType(C2_private, "typeof C2_private");
            return new C2_private();  // error
        }

        private f9_private(): C1_public {
AssertType(new C1_public(), "C1_public");
AssertType(C1_public, "typeof C1_public");
            return new C1_public();
        }

        public f10_public(): C1_public {
AssertType(new C1_public(), "C1_public");
AssertType(C1_public, "typeof C1_public");
            return new C1_public();
        }

        private f11_private(): C2_private {
AssertType(new C2_private(), "C2_private");
AssertType(C2_private, "typeof C2_private");
            return new C2_private();
        }

        public f12_public(): C2_private { // error
AssertType(new C2_private(), "C2_private");
AssertType(C2_private, "typeof C2_private");
            return new C2_private(); //error
        }
    }

    class C4_private {
        constructor (m1_c4_c1: C1_public);
        constructor (m1_c4_c2: C2_private);
        constructor (m1_c4_c1_2: any) {
        }
        private f1_private(m1_c4_f1_arg: C1_public) {
        }

        public f2_public(m1_c4_f2_arg: C1_public) {
        }

        private f3_private(m1_c4_f3_arg: C2_private) {
        }

        public f4_public(m1_c4_f4_arg: C2_private) {
        }


        private f5_private() {
AssertType(new C1_public(), "C1_public");
AssertType(C1_public, "typeof C1_public");
            return new C1_public();
        }

        public f6_public() {
AssertType(new C1_public(), "C1_public");
AssertType(C1_public, "typeof C1_public");
            return new C1_public();
        }

        private f7_private() {
AssertType(new C2_private(), "C2_private");
AssertType(C2_private, "typeof C2_private");
            return new C2_private();
        }

        public f8_public() {
AssertType(new C2_private(), "C2_private");
AssertType(C2_private, "typeof C2_private");
            return new C2_private();
        }


        private f9_private(): C1_public {
AssertType(new C1_public(), "C1_public");
AssertType(C1_public, "typeof C1_public");
            return new C1_public();
        }

        public f10_public(): C1_public {
AssertType(new C1_public(), "C1_public");
AssertType(C1_public, "typeof C1_public");
            return new C1_public();
        }

        private f11_private(): C2_private {
AssertType(new C2_private(), "C2_private");
AssertType(C2_private, "typeof C2_private");
            return new C2_private();
        }

        public f12_public(): C2_private {
AssertType(new C2_private(), "C2_private");
AssertType(C2_private, "typeof C2_private");
            return new C2_private();
        }
    }

    export class C5_public {
        constructor (m1_c5_c: C1_public) {
        }
    }
    
    class C6_private {
        constructor (m1_c6_c: C1_public) {
        }
    }
    export class C7_public {
        constructor (m1_c7_c: C2_private) { // error
        }
    }
    
    class C8_private {
        constructor (m1_c8_c: C2_private) {
        }
    }

    function f1_public(m1_f1_arg: C1_public) {
    }

    export function f2_public(m1_f2_arg: C1_public) {
    }

    function f3_public(m1_f3_arg: C2_private) {
    }

    export function f4_public(m1_f4_arg: C2_private) { // error
    }


    function f5_public() {
AssertType(new C1_public(), "C1_public");
AssertType(C1_public, "typeof C1_public");
        return new C1_public();
    }

    export function f6_public() {
AssertType(new C1_public(), "C1_public");
AssertType(C1_public, "typeof C1_public");
        return new C1_public();
    }

    function f7_public() {
AssertType(new C2_private(), "C2_private");
AssertType(C2_private, "typeof C2_private");
        return new C2_private();
    }

    export function f8_public() {
AssertType(new C2_private(), "C2_private");
AssertType(C2_private, "typeof C2_private");
        return new C2_private();  // error
    }


    function f9_private(): C1_public {
AssertType(new C1_public(), "C1_public");
AssertType(C1_public, "typeof C1_public");
        return new C1_public();
    }

    export function f10_public(): C1_public {
AssertType(new C1_public(), "C1_public");
AssertType(C1_public, "typeof C1_public");
        return new C1_public();
    }

    function f11_private(): C2_private {
AssertType(new C2_private(), "C2_private");
AssertType(C2_private, "typeof C2_private");
        return new C2_private();
    }

    export function f12_public(): C2_private { // error
AssertType(new C2_private(), "C2_private");
AssertType(C2_private, "typeof C2_private");
        return new C2_private(); //error
    }
}

class C6_public {
}

class C7_public {
    constructor (c7_c2: C6_public);
    constructor (c7_c1_2: any) {
    }
    private f1_private(c7_f1_arg: C6_public) {
    }

    public f2_public(c7_f2_arg: C6_public) {
    }

    private f5_private() {
AssertType(new C6_public(), "C6_public");
AssertType(C6_public, "typeof C6_public");
        return new C6_public();
    }

    public f6_public() {
AssertType(new C6_public(), "C6_public");
AssertType(C6_public, "typeof C6_public");
        return new C6_public();
    }

    private f9_private(): C6_public {
AssertType(new C6_public(), "C6_public");
AssertType(C6_public, "typeof C6_public");
        return new C6_public();
    }

    public f10_public(): C6_public {
AssertType(new C6_public(), "C6_public");
AssertType(C6_public, "typeof C6_public");
        return new C6_public();
    }
}

class C9_public {
    constructor (c9_c: C6_public) {
    }
}
  

function f4_public(f4_arg: C6_public) {
}



function f6_public() {
AssertType(new C6_public(), "C6_public");
AssertType(C6_public, "typeof C6_public");
    return new C6_public();
}


function f10_public(): C6_public {
AssertType(new C6_public(), "C6_public");
AssertType(C6_public, "typeof C6_public");
    return new C6_public();
}


