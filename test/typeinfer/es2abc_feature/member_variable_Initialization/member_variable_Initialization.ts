/*
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
 */

declare function AssertType(value:any, type:string):void;
{
    class A {
        a1 = 1;
        public a2 = 0.1;
        private a3 = "abc";
        public static a4 = "abc" + "efg";

        f() {
            AssertType(this.a3, "string");
        }
    }

    function f1() {
        let a = new A();
        AssertType(a.a1, "number");
        AssertType(a.a2, "number");
        AssertType(A.a4, "string");
    }

    class C {
        c1 : A[]
        static c2 = new A();
    }

    class B {
        b1 = new A();
        public static b2 = new A();
        b3 : number
        b4 = C.c2;
        constructor() {
            this.b3 = 3
        }
    }

    function f2() {
        let b = new B();
        AssertType(b.b1, "A");
        AssertType(B.b2, "A");
        AssertType(b.b3, "number");
        AssertType(b.b4, "A");
    }
}