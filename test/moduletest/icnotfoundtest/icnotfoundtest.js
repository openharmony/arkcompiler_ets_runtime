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

print("Test for NotFoud IC Case.");

// Test1: test not found ic
function Test1() {
    let obj1 = {};
    let obj2 = {};
    let obj3 = {};
    obj1.__proto__ = obj2;
    obj2.__proto__ = obj3;

    let sum = 0;
    function add(obj) {
        if (obj.x != undefined) {
            sum += obj.x;
        }
    }

    for (let i = 0 ; i < 200; i++) {
        add(obj1);
        if (i == 100) {
            obj3.x = 233;
        }
    }
    print("Test1 ans:", sum);
}

// Test2: test not found ic
function Test2() {
    let obj1 = {};
    let obj2 = {};
    let obj3 = {};
    obj1.__proto__ = obj2;

    let sum = 0;
    function add(obj) {
        if (obj.x != undefined) {
            sum += obj.x;
        }
    }

    for (let i = 0 ; i < 200; i++) {
        add(obj1);
        if (i == 100) {
            obj3.x = 233;
        }
        if (i == 105) {
            obj2.__proto__ = obj3;
        }
    }
    print("Test2 ans:", sum);
}

// Test3: test not found ic
function Test3() {
    let obj1 = {};
    let obj2 = {};
    let obj3 = {};
    obj1.__proto__ = obj2;

    let sum = 0;
    function add(obj) {
        if (obj.x != undefined) {
            sum += obj.x;
        }
    }

    for (let i = 0 ; i < 200; i++) {
        add(obj1);
        if (i == 100) {
            obj3.x = 233;
        }
        if (i == 105) {
            obj2.__proto__ = obj3;
        }
        if (i == 110) {
            obj2.x = 234;
        }
    }
    print("Test3 ans:", sum);
}

// Test4: test not found ic
function Test4() {
    let obj1 = {};
    let obj2 = {};
    let obj3 = {};
    obj1.__proto__ = obj2;
    obj2.__proto__ = obj3;

    let sum = 0;
    function add(obj) {
        if (obj.x != undefined) {
            sum += obj.x;
        }
    }

    for (let i = 0 ; i < 200; i++) {
        add(obj1);
        if (i == 100) {
            obj1.x = 233;
        }
        if (i == 105) {
            delete obj1.x;
        }
        if (i == 110) {
            obj3.x = 234;
        }
    }
    print("Test4 ans:", sum);
}

// Test5: test not found ic
function Test5() {
    let obj1 = {};
    let obj2 = {};
    let obj3 = {};
    obj1.__proto__ = obj2;
    obj2.__proto__ = obj3;

    let sum = 0;
    function add(obj) {
        if (obj.x != undefined) {
            sum += obj.x;
        }
    }
    
    obj2.x = 235;
    for (let i = 0 ; i < 200; i++) {
        add(obj1);
        if (i == 100) {
            obj1.x = 233;
        }
        if (i == 105) {
            delete obj1.x;
        }
    }
    print("Test5 ans:", sum);
}

// Test6: test not found ic (method on itself)
function Test6() {
    class F1 {}
    let f1 = new F1();
    let P1 = F1.prototype;
    let sum = 0;
    function add(f, num) {
        if (f != undefined) {
            sum += f(num);
        }
    }
    for (let i = 0; i < 200; i++) {
        add(f1.toNumber, "233");
        if (i === 100) {
            f1.toNumber = function (str) {
                return parseInt(str);
            };
        }
    }
    print("Test6 ans:", sum);
}

// Test7: test not found ic (method on prototype)
function Test7() {
    class F1 {}
    let f1 = new F1();
    let P1 = F1.prototype;
    let sum = 0;
    function add(f, num) {
        if (f != undefined) {
            sum += f(num);
        }
    }
    for (let i = 0; i < 200; i++) {
        add(f1.toNumber, "233");
        if (i === 100) {
            P1.toNumber = function (str) {
                return parseInt(str);
            };
        }
    }
    print("Test7 ans:", sum);
}

// Test8: test not found ic (method on Object's prototype)
function Test8() {
    class F1 {}
    let f1 = new F1();
    let P1 = F1.prototype;
    let sum = 0;
    function add(f, num) {
        if (f != undefined) {
            sum += f(num);
        }
    }
    for (let i = 0; i < 200; i++) {
        add(f1.toNumber, "233");
        if (i === 100) {
            Object.prototype.toNumber = function (str) {
                return parseInt(str);
            };
        }
    }
    print("Test8 ans:", sum);
}

// Test9: test not found ic (global object)
function Test9() {
    var obj1 = {};
    var obj2 = {};
    var obj3 = {};
    obj1.__proto__ = obj2;
    obj2.__proto__ = obj3;

    let sum = 0;
    function add(obj) {
        if (obj.x != undefined) {
            sum += obj.x;
        }
    }

    for (let i = 0 ; i < 200; i++) {
        add(obj1);
        if (i == 100) {
            obj3.x = 233;
        }
    }
    print("Test9 ans:", sum);
}

// Test10: test not found ic (ldobjbyvalue)
function Test10() {
    let obj1 = {};
    let obj2 = {};
    let obj3 = {};
    obj1.__proto__ = obj2;

    let sum = 0;
    function add(obj) {
        if (obj["x"] != undefined) {
            sum += obj["x"];
        }
    }

    for (let i = 0 ; i < 200; i++) {
        add(obj1);
        if (i == 100) {
            obj3.x = 233;
        }
        if (i == 105) {
            obj2.__proto__ = obj3;
        }
        if (i == 110) {
            obj2.x = 234;
        }
    }
    print("Test10 ans:", sum);
}

Test1();
Test2();
Test3();
Test4();
Test5();
Test6();
Test7();
Test8();
Test9();
Test10()