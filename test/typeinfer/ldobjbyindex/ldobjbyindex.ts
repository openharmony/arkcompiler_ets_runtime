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
    let arr : string[] = ['1'];
    let ans = arr[0];
    AssertType(ans, "string");

    let ans2 = arr["0"]
    AssertType(ans2, "string");

    class C {
        _array: Float32Array;
        constructor() {
            this._array = new Float32Array(2);
        }
    }

    function test(c:C) {
        AssertType(c._array[0], "double");
        AssertType(c._array[1], "double");
        AssertType(c._array["1"], "double");
    }

    let obj : {
        10 : number,
        "abc" : number,
        10.13 : number,
        9 : "bb",
        100.3e2 : string,
    } = {
        10 : 13,
        "abc" : 13,
        10.13 : 13,
        9 : "bb",
        100.3e2 : "abc"
    }
    let index = 10.13;
    let index02 = 100.3e2;
    AssertType(obj[10], "number");
    AssertType(obj[10.13], "number");
    AssertType(obj[9], "string");
    AssertType(obj[index], "number");
    AssertType(obj[index02], "string");

    class Stu {
        8 : number;
        static 9() : string {
            return "str";
        }

        10() : number {
            return 10;
        }

        static 9.1() : string {
            return "9.1";
        }

        10.1() : number {
            Stu[8] = 11;
            return 1;
        }
    }

    AssertType(Stu[9](), "string");
    AssertType(Stu[9.1](), "string");

    let stu = new Stu();
    let index2 = 10.1;
    AssertType(stu[10](), "number");
    AssertType(stu[10.1](), "number");
    AssertType(stu[8], "number");
    AssertType(stu[index2](), "number");
}
