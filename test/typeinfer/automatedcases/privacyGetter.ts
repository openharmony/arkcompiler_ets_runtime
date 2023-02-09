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

// === tests/cases/compiler/privacyGetter.ts ===
declare function AssertType(value:any, type:string):void;
export module m1 {
    export class C1_public {
        private f1() {
        }
    }

    class C2_private {
    }

    export class C3_public {
        private get p1_private() {
AssertType(new C1_public(), "C1_public");
AssertType(C1_public, "typeof C1_public");
            return new C1_public();
        }

        private set p1_private(m1_c3_p1_arg: C1_public) {
        }

        private get p2_private() {
AssertType(new C1_public(), "C1_public");
AssertType(C1_public, "typeof C1_public");
            return new C1_public();
        }

        private set p2_private(m1_c3_p2_arg: C1_public) {
        }

        private get p3_private() {
AssertType(new C2_private(), "C2_private");
AssertType(C2_private, "typeof C2_private");
            return new C2_private();
        }

        private set p3_private(m1_c3_p3_arg: C2_private) {
        }

        public get p4_public(): C2_private { // error
AssertType(new C2_private(), "C2_private");
AssertType(C2_private, "typeof C2_private");
            return new C2_private(); //error
        }

        public set p4_public(m1_c3_p4_arg: C2_private) { // error
        }
    }

    class C4_private {
        private get p1_private() {
AssertType(new C1_public(), "C1_public");
AssertType(C1_public, "typeof C1_public");
            return new C1_public();
        }

        private set p1_private(m1_c3_p1_arg: C1_public) {
        }

        private get p2_private() {
AssertType(new C1_public(), "C1_public");
AssertType(C1_public, "typeof C1_public");
            return new C1_public();
        }

        private set p2_private(m1_c3_p2_arg: C1_public) {
        }

        private get p3_private() {
AssertType(new C2_private(), "C2_private");
AssertType(C2_private, "typeof C2_private");
            return new C2_private();
        }

        private set p3_private(m1_c3_p3_arg: C2_private) {
        }

        public get p4_public(): C2_private {
AssertType(new C2_private(), "C2_private");
AssertType(C2_private, "typeof C2_private");
            return new C2_private();
        }

        public set p4_public(m1_c3_p4_arg: C2_private) {
        }
    }
}

module m2 {
    export class m2_C1_public {
        private f1() {
        }
    }

    class m2_C2_private {
    }

    export class m2_C3_public {
        private get p1_private() {
AssertType(new m2_C1_public(), "m2_C1_public");
AssertType(m2_C1_public, "typeof m2_C1_public");
            return new m2_C1_public();
        }

        private set p1_private(m2_c3_p1_arg: m2_C1_public) {
        }

        private get p2_private() {
AssertType(new m2_C1_public(), "m2_C1_public");
AssertType(m2_C1_public, "typeof m2_C1_public");
            return new m2_C1_public();
        }

        private set p2_private(m2_c3_p2_arg: m2_C1_public) {
        }

        private get p3_private() {
AssertType(new m2_C2_private(), "m2_C2_private");
AssertType(m2_C2_private, "typeof m2_C2_private");
            return new m2_C2_private();
        }

        private set p3_private(m2_c3_p3_arg: m2_C2_private) {
        }

        public get p4_public(): m2_C2_private {
AssertType(new m2_C2_private(), "m2_C2_private");
AssertType(m2_C2_private, "typeof m2_C2_private");
            return new m2_C2_private();
        }

        public set p4_public(m2_c3_p4_arg: m2_C2_private) {
        }
    }

    class m2_C4_private {
        private get p1_private() {
AssertType(new m2_C1_public(), "m2_C1_public");
AssertType(m2_C1_public, "typeof m2_C1_public");
            return new m2_C1_public();
        }

        private set p1_private(m2_c3_p1_arg: m2_C1_public) {
        }

        private get p2_private() {
AssertType(new m2_C1_public(), "m2_C1_public");
AssertType(m2_C1_public, "typeof m2_C1_public");
            return new m2_C1_public();
        }

        private set p2_private(m2_c3_p2_arg: m2_C1_public) {
        }

        private get p3_private() {
AssertType(new m2_C2_private(), "m2_C2_private");
AssertType(m2_C2_private, "typeof m2_C2_private");
            return new m2_C2_private();
        }

        private set p3_private(m2_c3_p3_arg: m2_C2_private) {
        }

        public get p4_public(): m2_C2_private {
AssertType(new m2_C2_private(), "m2_C2_private");
AssertType(m2_C2_private, "typeof m2_C2_private");
            return new m2_C2_private();
        }

        public set p4_public(m2_c3_p4_arg: m2_C2_private) {
        }
    }
}

class C5_private {
    private f() {
    }
}

export class C6_public {
}

export class C7_public {
    private get p1_private() {
AssertType(new C6_public(), "C6_public");
AssertType(C6_public, "typeof C6_public");
        return new C6_public();
    }

    private set p1_private(m1_c3_p1_arg: C6_public) {
    }

    private get p2_private() {
AssertType(new C6_public(), "C6_public");
AssertType(C6_public, "typeof C6_public");
        return new C6_public();
    }

    private set p2_private(m1_c3_p2_arg: C6_public) {
    }

    private get p3_private() {
AssertType(new C5_private(), "C5_private");
AssertType(C5_private, "typeof C5_private");
        return new C5_private();
    }

    private set p3_private(m1_c3_p3_arg: C5_private) {
    }

    public get p4_public(): C5_private { // error
AssertType(new C5_private(), "C5_private");
AssertType(C5_private, "typeof C5_private");
        return new C5_private(); //error
    }

    public set p4_public(m1_c3_p4_arg: C5_private) { // error
    }
}

class C8_private {
    private get p1_private() {
AssertType(new C6_public(), "C6_public");
AssertType(C6_public, "typeof C6_public");
        return new C6_public();
    }

    private set p1_private(m1_c3_p1_arg: C6_public) {
    }

    private get p2_private() {
AssertType(new C6_public(), "C6_public");
AssertType(C6_public, "typeof C6_public");
        return new C6_public();
    }

    private set p2_private(m1_c3_p2_arg: C6_public) {
    }

    private get p3_private() {
AssertType(new C5_private(), "C5_private");
AssertType(C5_private, "typeof C5_private");
        return new C5_private();
    }

    private set p3_private(m1_c3_p3_arg: C5_private) {
    }

    public get p4_public(): C5_private {
AssertType(new C5_private(), "C5_private");
AssertType(C5_private, "typeof C5_private");
        return new C5_private();
    }

    public set p4_public(m1_c3_p4_arg: C5_private) {
    }
}

