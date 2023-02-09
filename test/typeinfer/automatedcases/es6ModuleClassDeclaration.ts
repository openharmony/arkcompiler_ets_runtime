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

// === tests/cases/compiler/es6ModuleClassDeclaration.ts ===
declare function AssertType(value:any, type:string):void;
export class c {
    constructor() {
    }
    private x = 10;
    public y = 30;
    static k = 20;
    private static l = 30;
    private method1() {
    }
    public method2() {
    }
    static method3() {
    }
    private static method4() {
    }
}
class c2 {
    constructor() {
    }
    private x = 10;
    public y = 30;
    static k = 20;
    private static l = 30;
    private method1() {
    }
    public method2() {
    }
    static method3() {
    }
    private static method4() {
    }
}
new c();
AssertType(new c(), "c");
AssertType(c, "typeof c");

new c2();
AssertType(new c2(), "c2");
AssertType(c2, "typeof c2");

export module m1 {
    export class c3 {
        constructor() {
        }
        private x = 10;
        public y = 30;
        static k = 20;
        private static l = 30;
        private method1() {
        }
        public method2() {
        }
        static method3() {
        }
        private static method4() {
        }
    }
    class c4 {
        constructor() {
        }
        private x = 10;
        public y = 30;
        static k = 20;
        private static l = 30;
        private method1() {
        }
        public method2() {
        }
        static method3() {
        }
        private static method4() {
        }
    }
    new c();
    new c2();
    new c3();
    new c4();
}
module m2 {
    export class c3 {
        constructor() {
        }
        private x = 10;
        public y = 30;
        static k = 20;
        private static l = 30;
        private method1() {
        }
        public method2() {
        }
        static method3() {
        }
        private static method4() {
        }
    }
    class c4 {
        constructor() {
        }
        private x = 10;
        public y = 30;
        static k = 20;
        private static l = 30;
        private method1() {
        }
        public method2() {
        }
        static method3() {
        }
        private static method4() {
        }
    }
    new c();
    new c2();
    new c3();
    new c4();
    new m1.c3();
}

