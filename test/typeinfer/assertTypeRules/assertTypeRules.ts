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

/*
 * | AssertType rules           |                                                 |
 * | class Student {}           |   AssertType(Student, "typeof Student")         |
 * | let stu = new Studnet()    |   AssertType(stu, "Studnet")                    |
 * | interface face {}          |   AssertType(face, "interface")                 |
 * | function add(num: number, 
 * |   str: string) : string {} |   AssertType(add, "(number, string) => string") |
 * | let arr : number[] = [1]   |   AssertType(arr, "number[]")                   |
 * | let test : number | string |   AssertType(test, "union")                     |
 * | let num = 1;               |   AssertType(num, "int")                        |
 * | let str : string = ""      |   AssertType(num, "string")                     |
 */
declare function AssertType(value:any, type:string):void;
{
    class Student {}
    let stu = new Student();
    AssertType(Student, "typeof Student")
    AssertType(stu, "Student")

    interface Face {}
    function testInterface(obj : Face) {
        AssertType(obj, "interface");
    }

    function add(num : number, str : string) : string {
        return num + str;
    }
    AssertType(add(1, ""), "string");
    AssertType(add, "(number, string) => string");

    let arr : number[] = [1];
    AssertType(arr, "number[]");
    let vec : number[][] = [[1]];
    AssertType(vec, "number[][]");

    let num = 1;
    AssertType(num, "int");

    let str : string = "";
    AssertType(str, "string");
}
