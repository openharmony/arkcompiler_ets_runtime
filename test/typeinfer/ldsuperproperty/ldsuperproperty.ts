/*
 * Copyright (c) 2022 Huawei Device Co., Ltd.
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
        a1 : number;
        a2 : string;
        static a3 : string = "abc";
        constructor(a11 : number) {
            this.a1 = a11;
            this.a2 = "abc";
        }
        getA() : number {
            return this.a1;
        }
        get geta2() : string {
            return this.a2;
        }
        2() : string {
            return this.a2;
        }
    }
    
    class B extends A {
        b  : number;
        constructor(a1 : number, b1 : number) {
            super(a1);
            this.b = b1;
        }
        getD() {
            let d1 = super.a1;
            AssertType(d1, "undefined");
            let d2 = super.a2;
            AssertType(d2, "undefined");
            let d3 = super.geta2;
            AssertType(d3, "string");
            let d4 = super.getA();
            AssertType(d4, "number");
        }
        static getA3() {
            let d5 = super.a3;
            AssertType(d5, "string");
        }
        getc() {
            let a = "getA";
            let b = 2;
            let a2 = super[a]();
            AssertType(a2, "number")
            let a3 = super[b]();
            AssertType(a3, "string")
            let a4 = this[a]();
            AssertType(a4, "number")
            let a5 = this[b]();
            AssertType(a5, "string")
        }
    }
    
    let d = new B(1,2);
    d.getD();
    B.getA3();
}
