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

import {Test1} from "./export1"
import {Test2} from "./export2"

let t1 = new Test1("abc");
AssertType(t1, "Test1");
AssertType(t1.value1, "string");

let t2 = new Test2("123", 2);
AssertType(t2, "Test2");
AssertType(t2.value1, "string");
AssertType(t2.value2, "number");

class Test3 extends Test2 {
    value3:boolean;
    constructor(value1:string, value2:number, value3:boolean) {
        super(value1, value2);
        this.value3 = value3;
    }
}

let t3 = new Test3("123", 456, false);
AssertType(t3, "Test3");
AssertType(t3.value1, "string");
AssertType(t3.value2, "number");
AssertType(t3.value3, "boolean");
AssertType(t3.foo(), "string");

