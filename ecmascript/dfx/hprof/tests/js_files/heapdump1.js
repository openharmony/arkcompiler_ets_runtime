/*
 * Copyright (c) 2025 Huawei Device Co., Ltd.
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


// anonymous
let anonymous = () => {
    print('this is anonymous function');
};
globalThis.testAnonymous = anonymous;

// function
function test() {
    print('this is test function');
}
globalThis.testFunction = test;

function ObjectFun() {
    print('this is objectFun function');
}

class Person {
    constructor(name, age) {
      this.name = name;
      this.age = age;
    }

    greet() {
      print(`Hello, my name is ${this.name} and I'm ${this.age} years old.`);
    }
}


//object
let obj = {
    objectClass: new Person('Alice', 25),
    objFun: new ObjectFun(),

    testObject() {
        objFun();
        let name = objectClass.name;
        print('this object name is JSObject');
    }

};
globalThis.testObj = obj;

// class
class Student {
    constructor(name, grade) {
        this.name = name;
        this.grade = grade;
    }

    getMessage() {
        print(`Hello, his name is ${this.name} and he's  in ${this.grade} grade.`);
    }
}
globalThis.testClass = new Student('Davi', 1);

globalThis.arrayTest = new Array(33);


