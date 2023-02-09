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

// === tests/cases/compiler/privacyGloVar.ts ===
declare function AssertType(value:any, type:string):void;
module m1 {
    export class C1_public {
        private f1() {
        }
    }

    class C2_private {
    }

    export class C3_public {
        private C3_v1_private: C1_public;
        public C3_v2_public: C1_public;
        private C3_v3_private: C2_private;
        public C3_v4_public: C2_private; // error

        private C3_v11_private = new C1_public();
        public C3_v12_public = new C1_public();
        private C3_v13_private = new C2_private();
        public C3_v14_public = new C2_private(); // error

        private C3_v21_private: C1_public = new C1_public();
        public C3_v22_public: C1_public = new C1_public();
        private C3_v23_private: C2_private = new C2_private();
        public C3_v24_public: C2_private = new C2_private(); // error
    }

    class C4_public {
        private C4_v1_private: C1_public;
        public C4_v2_public: C1_public;
        private C4_v3_private: C2_private;
        public C4_v4_public: C2_private;

        private C4_v11_private = new C1_public();
        public C4_v12_public = new C1_public();
        private C4_v13_private = new C2_private();
        public C4_v14_public = new C2_private();

        private C4_v21_private: C1_public = new C1_public();
        public C4_v22_public: C1_public = new C1_public();
        private C4_v23_private: C2_private = new C2_private();
        public C4_v24_public: C2_private = new C2_private();
    }

    let m1_v1_private: C1_public;
    export let m1_v2_public: C1_public;
    let m1_v3_private: C2_private;
    export let m1_v4_public: C2_private; // error

    let m1_v11_private = new C1_public();
    export let m1_v12_public = new C1_public();
    let m1_v13_private = new C2_private();
    export let m1_v14_public = new C2_private(); //error 

    let m1_v21_private: C1_public = new C1_public();
    export let m1_v22_public: C1_public = new C1_public();
    let m1_v23_private: C2_private = new C2_private();
    export let m1_v24_public: C2_private = new C2_private(); // error
}

class glo_C1_public {
    private f1() {
    }
}

class glo_C3_public {
    private glo_C3_v1_private: glo_C1_public;
    public glo_C3_v2_public: glo_C1_public;

    private glo_C3_v11_private = new glo_C1_public();
    public glo_C3_v12_public = new glo_C1_public();

    private glo_C3_v21_private: glo_C1_public = new glo_C1_public();
    public glo_C3_v22_public: glo_C1_public = new glo_C1_public();
}


let glo_v2_public: glo_C1_public;
AssertType(glo_v2_public, "glo_C1_public");

let glo_v12_public = new glo_C1_public();
AssertType(glo_v12_public, "glo_C1_public");
AssertType(new glo_C1_public(), "glo_C1_public");
AssertType(glo_C1_public, "typeof glo_C1_public");

let glo_v22_public: glo_C1_public = new glo_C1_public();
AssertType(glo_v22_public, "glo_C1_public");
AssertType(new glo_C1_public(), "glo_C1_public");
AssertType(glo_C1_public, "typeof glo_C1_public");


