/*
 * Copyright (c) 2026 Huawei Device Co., Ltd.
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
 * @tc.name:loadicclass
 * @tc.desc:test loadic on class objects
 * @tc.type: FUNC
 * @tc.require: issueI7UTOA
 */

// Test basic class instance property load
function testBasicClassInstanceProperty() {
    class Person {
        constructor(name) {
            this.name = name;
        }
    }
    const person = new Person("Alice");
    for (let i = 0; i < 100; i++) {
        let res = person.name;
        assert_equal(res, "Alice");
    }
}
testBasicClassInstanceProperty();

// Test class method call
function testClassMethodCall() {
    class Calculator {
        add(a, b) {
            return a + b;
        }
        subtract(a, b) {
            return a - b;
        }
    }
    const calc = new Calculator();
    for (let i = 0; i < 100; i++) {
        let res1 = calc.add(5, 3);
        let res2 = calc.subtract(5, 3);
        assert_equal(res1, 8);
        assert_equal(res2, 2);
    }
}
testClassMethodCall();

// Test class getter
function testClassGetter() {
    class Rectangle {
        constructor(width, height) {
            this.width = width;
            this.height = height;
        }
        get area() {
            return this.width * this.height;
        }
    }
    const rect = new Rectangle(5, 10);
    for (let i = 0; i < 100; i++) {
        let res = rect.area;
        assert_equal(res, 50);
    }
}
testClassGetter();

// Test class setter
function testClassSetter() {
    class Temperature {
        constructor() {
            this._celsius = 0;
        }
        set celsius(value) {
            this._celsius = value;
        }
        get celsius() {
            return this._celsius;
        }
    }
    const temp = new Temperature();
    for (let i = 0; i < 100; i++) {
        temp.celsius = i;
        let res = temp.celsius;
        assert_equal(res, i);
    }
}
testClassSetter();

// Test class static method
function testClassStaticMethod() {
    class MathUtils {
        static add(a, b) {
            return a + b;
        }
        static multiply(a, b) {
            return a * b;
        }
    }
    for (let i = 0; i < 100; i++) {
        let res1 = MathUtils.add(2, 3);
        let res2 = MathUtils.multiply(2, 3);
        assert_equal(res1, 5);
        assert_equal(res2, 6);
    }
}
testClassStaticMethod();

// Test class static property
function testClassStaticProperty() {
    class Config {
        static get version() {
            return "1.0.0";
        }
    }
    for (let i = 0; i < 100; i++) {
        let res = Config.version;
        assert_equal(res, "1.0.0");
    }
}
testClassStaticProperty();

// Test class inheritance
function testClassInheritance() {
    class Animal {
        constructor(name) {
            this.name = name;
        }
        speak() {
            return this.name + " makes a sound";
        }
    }
    class Dog extends Animal {
        speak() {
            return this.name + " barks";
        }
    }
    const dog = new Dog("Buddy");
    for (let i = 0; i < 100; i++) {
        let res = dog.speak();
        assert_equal(res, "Buddy barks");
    }
}
testClassInheritance();

// Test class super call
function testClassSuperCall() {
    class Animal {
        constructor(name) {
            this.name = name;
        }
    }
    class Dog extends Animal {
        constructor(name, breed) {
            super(name);
            this.breed = breed;
        }
    }
    const dog = new Dog("Buddy", "Golden Retriever");
    for (let i = 0; i < 100; i++) {
        let res1 = dog.name;
        let res2 = dog.breed;
        assert_equal(res1, "Buddy");
        assert_equal(res2, "Golden Retriever");
    }
}
testClassSuperCall();

// Test class with private fields
function testClassPrivateFields() {
    class BankAccount {
        #balance = 0;
        deposit(amount) {
            this.#balance += amount;
            return this.#balance;
        }
        getBalance() {
            return this.#balance;
        }
    }
    const account = new BankAccount();
    for (let i = 0; i < 100; i++) {
        account.deposit(10);
        let res = account.getBalance();
        assert_equal(res, (i + 1) * 10);
    }
}
testClassPrivateFields();

// Test class with symbol properties
function testClassSymbolProperties() {
    const secret = Symbol("secret");
    class MyClass {
        constructor() {
            this[secret] = 42;
        }
        getSecret() {
            return this[secret];
        }
    }
    const obj = new MyClass();
    for (let i = 0; i < 100; i++) {
        let res = obj.getSecret();
        assert_equal(res, 42);
    }
}
testClassSymbolProperties();

// Test class name property
function testClassNameProperty() {
    class MyClass {}
    for (let i = 0; i < 100; i++) {
        let res = MyClass.name;
        assert_equal(res, "MyClass");
    }
}
testClassNameProperty();

// Test class prototype property
function testClassPrototypeProperty() {
    class MyClass {}
    for (let i = 0; i < 100; i++) {
        let res = MyClass.prototype;
        assert_equal(typeof res, "object");
    }
}
testClassPrototypeProperty();

// Test class constructor property
function testClassConstructorProperty() {
    class MyClass {}
    const obj = new MyClass();
    for (let i = 0; i < 100; i++) {
        let res = obj.constructor;
        assert_equal(res, MyClass);
    }
}
testClassConstructorProperty();

// Test multiple instances of same class
function testMultipleClassInstances() {
    class Counter {
        constructor() {
            this.count = 0;
        }
        increment() {
            return ++this.count;
        }
    }
    const counters = [];
    for (let i = 0; i < 10; i++) {
        counters.push(new Counter());
    }
    for (let i = 0; i < 100; i++) {
        for (let j = 0; j < counters.length; j++) {
            let res = counters[j].increment();
            assert_equal(res, i + 1);
        }
    }
}
testMultipleClassInstances();

// Test class instanceof check
function testClassInstanceof() {
    class Animal {}
    class Dog extends Animal {}
    const dog = new Dog();
    for (let i = 0; i < 100; i++) {
        let res1 = dog instanceof Dog;
        let res2 = dog instanceof Animal;
        let res3 = dog instanceof Object;
        assert_equal(res1, true);
        assert_equal(res2, true);
        assert_equal(res3, true);
    }
}
testClassInstanceof();

// Test class with method chaining
function testClassMethodChaining() {
    class Builder {
        constructor() {
            this.value = 0;
        }
        add(n) {
            this.value += n;
            return this;
        }
        multiply(n) {
            this.value *= n;
            return this;
        }
        getValue() {
            return this.value;
        }
    }
    for (let i = 0; i < 100; i++) {
        // Create a new builder each iteration to test IC with fresh objects
        const builder = new Builder();
        let res = builder.add(5).multiply(2).getValue();
        assert_equal(res, 10);
    }
}
testClassMethodChaining();

// Test class with computed property names
function testClassComputedPropertyNames() {
    const prefix = "method";
    class MyClass {
        [prefix + "1"]() {
            return 1;
        }
        [prefix + "2"]() {
            return 2;
        }
    }
    const obj = new MyClass();
    for (let i = 0; i < 100; i++) {
        let res1 = obj.method1();
        let res2 = obj.method2();
        assert_equal(res1, 1);
        assert_equal(res2, 2);
    }
}
testClassComputedPropertyNames();

// Test class with arrow function property
function testClassArrowFunctionProperty() {
    class MyClass {
        constructor() {
            this.multiplier = 2;
        }
        multiply = (n) => n * this.multiplier;
    }
    const obj = new MyClass();
    for (let i = 0; i < 100; i++) {
        let res = obj.multiply(5);
        assert_equal(res, 10);
    }
}
testClassArrowFunctionProperty();

test_end();
