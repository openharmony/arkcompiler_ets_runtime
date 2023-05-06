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

class A{};
class Test<T1, T2, T3, T4> {
    value1:T1;
    value2:T2;
    value3:T3;
    value4:T4;
    constructor(value1:T1, value2:T2, value3:T3, value4:T4){
        this.value1 = value1;
        this.value2 = value2;
        this.value3 = value3;
        this.value4 = value4;
    }
}

let obj = new Test<number, number, string, A> (1, 2, "", new A());

AssertType(obj, "Test");
AssertType(obj.value1, "number");
AssertType(obj.value2, "number");
AssertType(obj.value3, "string");
AssertType(obj.value4, "A");
