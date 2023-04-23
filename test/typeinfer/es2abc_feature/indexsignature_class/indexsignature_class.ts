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
    class TestNumber {
        [v:number]:string;
        value:number;
        constructor(value:number) {
            this.value = value;
        }
    }

    class TestString {
        [v:string]:number;
        value:number;
        constructor(value:number) {
            this.value = value;
        }
    }

    let t1:TestNumber = new TestNumber(123);
    t1[1] = "abc";
    t1[2] = "def";

    let t2:TestString = new TestString(123);
    t2.value1 = 123;
    t2.value2 = 456;
    let t3:TestString = new TestString(123);
    t3.value1 = 456;
    t3.value2 = 2;

    AssertType(t1.value, "number");
    AssertType(t1[1], "string");
    AssertType(t1[2], "string");
    AssertType(t2.value, "number");
    AssertType(t2.value1, "number");
    AssertType(t2.value2, "number");
    AssertType(t3.value1, "number");
    AssertType(t3.value2, "number");
}
