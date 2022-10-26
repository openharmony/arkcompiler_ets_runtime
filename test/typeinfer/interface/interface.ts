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
    class low {
        lowP : string;
        constructor(l : string) {
            this.lowP = l;
        }
        lowMethod() : string {
            return "";
        }
    }

    class subC extends low {
        claN : number
        constructor(tmp : number) {
            super("");
            this.claN = tmp;
        }
        subMethod() : string {
            return "";
        }
    }

    interface A extends low, subC { 
        bb : number
    }

    interface C extends A {}
    interface D extends C {}
    interface common extends D {
        male : boolean;
        isMale() : boolean;
    }

    interface base extends common {
        age : number;
        getAge() : number;
    }

    class otherClass {
        oth : number;
        constructor(tmp : number) {
            this.oth = tmp;
        }
        otherMethod() : string{
            return "";
        }
    }

    interface otherInterface extends otherClass{
        face : number;
    }

    interface other extends otherInterface {
        height : number;
        getHeight() : number;
    }

    interface student extends base, other{
        name : string;
        getName() : string;
    }

    let stu : student = {
        face : 1,
        oth : 1,
        otherMethod() {
            return "";
        },
        lowP : "hh",
        lowMethod() {return ""},
        claN : 1,
        subMethod() {return ""},
        bb : 2,
        name :"hello",
        getName() {
            return this.name;
        },
        age : 10,
        getAge() {
            return this.age;
        },
        male : true,
        isMale() {
            return this.male;
        },
        height : 100,
        getHeight() {
            return this.height;
        }

    };

    // test ldobjbyname for interface
    AssertType(stu.lowP, "string");
    AssertType(stu.lowMethod(), "string");
    AssertType(stu.subMethod(), "string");
    AssertType(stu.claN, "number");
    AssertType(stu.bb, "number");
    AssertType(stu.name, "string");
    AssertType(stu.getName(), "string");
    AssertType(stu.age, "number");
    AssertType(stu.getAge(), "number");
    AssertType(stu.male, "boolean");
    AssertType(stu.isMale(), "boolean");
    AssertType(stu.height, "number");
    AssertType(stu.getHeight(), "number");
}