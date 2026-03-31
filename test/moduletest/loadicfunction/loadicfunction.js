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
 * @tc.name:loadicfunction
 * @tc.desc:test loadic on function objects
 * @tc.type: FUNC
 * @tc.require: issueI7UTOA
 */

// Test function name property load
function testFunctionNameProperty() {
    function myFunction() {}
    for (let i = 0; i < 100; i++) {
        let res = myFunction.name;
        assert_equal(res, "myFunction");
    }
}
testFunctionNameProperty();

// Test function length property load
function testFunctionLengthProperty() {
    function func1() {}
    function func2(a) {}
    function func3(a, b, c) {}
    for (let i = 0; i < 100; i++) {
        let res1 = func1.length;
        let res2 = func2.length;
        let res3 = func3.length;
        assert_equal(res1, 0);
        assert_equal(res2, 1);
        assert_equal(res3, 3);
    }
}
testFunctionLengthProperty();

// Test function prototype property load
function testFunctionPrototypeProperty() {
    function MyClass() {}
    for (let i = 0; i < 100; i++) {
        let res = MyClass.prototype;
        assert_equal(typeof res, "object");
    }
}
testFunctionPrototypeProperty();

// Test function constructor property load
function testFunctionConstructorProperty() {
    function foo() {}
    for (let i = 0; i < 100; i++) {
        let res = foo.constructor;
        assert_equal(res, Function);
    }
}
testFunctionConstructorProperty();

// Test function call method load
function testFunctionCallMethod() {
    function foo() { return 42; }
    for (let i = 0; i < 100; i++) {
        let res = foo.call;
        assert_equal(typeof res, "function");
    }
}
testFunctionCallMethod();

// Test function apply method load
function testFunctionApplyMethod() {
    function foo() { return 42; }
    for (let i = 0; i < 100; i++) {
        let res = foo.apply;
        assert_equal(typeof res, "function");
    }
}
testFunctionApplyMethod();

// Test function bind method load
function testFunctionBindMethod() {
    function foo() { return 42; }
    for (let i = 0; i < 100; i++) {
        let res = foo.bind;
        assert_equal(typeof res, "function");
    }
}
testFunctionBindMethod();

// Test function toString method load
function testFunctionToStringMethod() {
    function foo() { return 42; }
    for (let i = 0; i < 100; i++) {
        let res = foo.toString;
        assert_equal(typeof res, "function");
    }
}
testFunctionToStringMethod();

// Test function call with different receivers
function testFunctionCallWithReceivers() {
    function getX() { return this.x; }
    const obj1 = { x: 1 };
    const obj2 = { x: 2 };
    for (let i = 0; i < 100; i++) {
        let res1 = getX.call(obj1);
        let res2 = getX.call(obj2);
        assert_equal(res1, 1);
        assert_equal(res2, 2);
    }
}
testFunctionCallWithReceivers();

// Test function apply with arrays
function testFunctionApplyWithArrays() {
    function sum(a, b, c) { return a + b + c; }
    for (let i = 0; i < 100; i++) {
        let res = sum.apply(null, [1, 2, 3]);
        assert_equal(res, 6);
    }
}
testFunctionApplyWithArrays();

// Test function bind and call
function testFunctionBindAndCall() {
    function greet(greeting) { return greeting + " " + this.name; }
    const obj = { name: "World" };
    const boundFunc = greet.bind(obj);
    for (let i = 0; i < 100; i++) {
        let res = boundFunc("Hello");
        assert_equal(res, "Hello World");
    }
}
testFunctionBindAndCall();

// Test arrow function property load
function testArrowFunctionProperties() {
    const arrow = () => 42;
    for (let i = 0; i < 100; i++) {
        let res1 = arrow.length;
        let res2 = arrow.name;
        assert_equal(res1, 0);
        assert_equal(res2, "arrow");
    }
}
testArrowFunctionProperties();

// Test anonymous function name property
function testAnonymousFunctionName() {
    const anon = function() {};
    for (let i = 0; i < 100; i++) {
        let res = anon.name;
        assert_equal(res, "anon");
    }
}
testAnonymousFunctionName();

// Test function as object with custom properties
function testFunctionWithCustomProperties() {
    function foo() {}
    foo.customProp = 123;
    foo.customMethod = function() { return 456; };
    for (let i = 0; i < 100; i++) {
        let res1 = foo.customProp;
        let res2 = foo.customMethod();
        assert_equal(res1, 123);
        assert_equal(res2, 456);
    }
}
testFunctionWithCustomProperties();

// Test function prototype chain
function testFunctionPrototypeChain() {
    function Foo() {}
    Foo.prototype.greet = function() { return "Hello"; };
    const obj = new Foo();
    for (let i = 0; i < 100; i++) {
        let res = obj.greet();
        assert_equal(res, "Hello");
    }
}
testFunctionPrototypeChain();

// Test function constructor with new
function testFunctionConstructorWithNew() {
    function Person(name) {
        this.name = name;
    }
    for (let i = 0; i < 100; i++) {
        let person = new Person("Alice");
        let res = person.name;
        assert_equal(res, "Alice");
    }
}
testFunctionConstructorWithNew();

// Test function arguments object
function testFunctionArgumentsObject() {
    function foo() {
        return arguments.length;
    }
    for (let i = 0; i < 100; i++) {
        let res = foo(1, 2, 3);
        assert_equal(res, 3);
    }
}
testFunctionArgumentsObject();

// Test function with rest parameters
function testFunctionRestParameters() {
    function foo(...args) {
        return args.length;
    }
    for (let i = 0; i < 100; i++) {
        let res = foo(1, 2, 3, 4, 5);
        assert_equal(res, 5);
    }
}
testFunctionRestParameters();

// Test function with default parameters
function testFunctionDefaultParameters() {
    function foo(a = 10, b = 20) {
        return a + b;
    }
    for (let i = 0; i < 100; i++) {
        let res1 = foo();
        let res2 = foo(5);
        assert_equal(res1, 30);
        assert_equal(res2, 25);
    }
}
testFunctionDefaultParameters();

// Test function caller property
function testFunctionCallerProperty() {
    function inner() {
        return inner.caller;
    }
    function outer() {
        return inner();
    }
    // In strict mode, accessing caller property throws TypeError
    // This is expected behavior per ECMAScript spec
    let errorThrown = false;
    try {
        outer();
    } catch (e) {
        errorThrown = true;
    }
    assert_equal(errorThrown, true);
}
testFunctionCallerProperty();

// Test recursive function
function testRecursiveFunction() {
    function factorial(n) {
        if (n <= 1) return 1;
        return n * factorial(n - 1);
    }
    for (let i = 0; i < 100; i++) {
        let res = factorial(5);
        assert_equal(res, 120);
    }
}
testRecursiveFunction();

// Test nested function
function testNestedFunction() {
    function outer() {
        function inner() {
            return 42;
        }
        return inner();
    }
    for (let i = 0; i < 100; i++) {
        let res = outer();
        assert_equal(res, 42);
    }
}
testNestedFunction();

// Test closure function
function testClosureFunction() {
    function makeCounter() {
        let count = 0;
        return function() {
            return ++count;
        };
    }
    const counter = makeCounter();
    for (let i = 0; i < 100; i++) {
        let res = counter();
        assert_equal(res, i + 1);
    }
}
testClosureFunction();

// Test function bind multiple times
function testFunctionBindMultiple() {
    function greet() { return this.name; }
    const obj1 = { name: "First" };
    const obj2 = { name: "Second" };
    const bound1 = greet.bind(obj1);
    const bound2 = bound1.bind(obj2);
    for (let i = 0; i < 100; i++) {
        let res1 = bound1();
        let res2 = bound2();
        assert_equal(res1, "First");
        assert_equal(res2, "First");
    }
}
testFunctionBindMultiple();

// Test function with Symbol.hasInstance
function testFunctionSymbolHasInstance() {
    function Foo() {}
    const obj = new Foo();
    for (let i = 0; i < 100; i++) {
        let res = obj instanceof Foo;
        assert_equal(res, true);
    }
}
testFunctionSymbolHasInstance();

test_end();
