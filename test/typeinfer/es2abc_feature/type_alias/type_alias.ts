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

type T1 = string;
let a:T1 = "AOT";
AssertType(a, "string");

interface Test { 
    value:number;
    flag:boolean;
}
type T2 = Test;
let inst:T2 = {value: 123, flag: true};
AssertType(inst.value, "number");
AssertType(inst.flag, "boolean");

class C {
    value:number;
    constructor(value:number) {
        this.value = value;
    }
}
type T3 = C;
let c:T3 = new C(10);
AssertType(c.value, "number");

type T4 = (a:number) => number;
let f:T4 = (a) => {
    return a
};
AssertType(f, "(number) => number");
AssertType(f(123), "number");

type T5 = {name:string, age:number};
let obj:T5 = {name:"abc", age:10};
AssertType(obj.name, "string");
AssertType(obj.age, "number");
