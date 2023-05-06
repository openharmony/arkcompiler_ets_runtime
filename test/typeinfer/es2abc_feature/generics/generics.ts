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

class Test<T> {
    value:T;
    name:string;
    constructor(value:T, name:string){
        this.value = value;
        this.name = name;
    }
    foo ():number {
        return 123;
    }
    bar ():T {
        return this.value;
    }
}

let numberObj = new Test<number> (123, "abc");
let stringObj = new Test<string> ("aot", "abc");
let booleanObj = new Test<boolean> (true, "abc");

AssertType(numberObj, "Test");
AssertType(numberObj.value, "number");
AssertType(numberObj.name, "string");
AssertType(numberObj.foo(), "number");
AssertType(numberObj.bar(), "number");
AssertType(numberObj.foo, "() => number");
AssertType(numberObj.bar, "() => number");

AssertType(stringObj, "Test");
AssertType(stringObj.value, "string");
AssertType(stringObj.name, "string");
AssertType(stringObj.foo(), "number");
AssertType(stringObj.bar(), "string");
AssertType(stringObj.foo, "() => number");
AssertType(stringObj.bar, "() => string");

AssertType(booleanObj, "Test");
AssertType(booleanObj.value, "boolean");
AssertType(booleanObj.name, "string");
AssertType(booleanObj.foo(), "number");
AssertType(booleanObj.bar(), "boolean");
AssertType(booleanObj.foo, "() => number");
AssertType(booleanObj.bar, "() => boolean");
