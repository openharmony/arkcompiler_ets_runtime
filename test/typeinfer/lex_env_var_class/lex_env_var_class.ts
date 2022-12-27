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
    class Student {
        name : string;
        age : number;
        constructor(name : string, age : number) {
            this.name = name;
            this.age = age;
        }
        static isMale : boolean = true; 
    }
    function f():any {
        let stu1 = new Student("Jack", 10);
        AssertType(stu1, "Student");
        let n1 = stu1.name;
        AssertType(n1, "string");
        let a1 = stu1.age;
        AssertType(a1, "number");
        AssertType(Student.isMale, "boolean");
        return a1;
    }

    let stu2 = new Student("Tom", 10);
    AssertType(stu2, "Student");
    let n2 = stu2.name;
    AssertType(n2, "string");
    let a2 = stu2.age;
    AssertType(a2, "number");
    AssertType(Student.isMale, "boolean");
}
