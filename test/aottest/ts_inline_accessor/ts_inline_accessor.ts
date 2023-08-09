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

declare function print(arg:any):string;

class Person {
    name: string;
    age: number;

    constructor(name: string, age: number) {
        this.name = name;
        this.age = age;
    }

    get GetName() {
        return this.name;
    }
    set SetName(str: string ) {
        this.name = str;
    }

    get GetAge() {
        return this.age;
    }

    set SetAge(age: number) {
        this.age = age;
        this.foo();
    }

    foo() {
        print("foo");
    }

}

let p = new Person("xiaoming", 18);
print(p.GetName)
p.SetName = "xiaohong";
print(p.GetName);
p.SetAge = 20;
print(p.GetAge);
