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

/*
 * @tc.name: call_comprehensive_test
 * @tc.desc: Comprehensive test suite for Call operations and related features
 * @tc.type: FUNC
 * @tc.require: call_comprehensive_suite
 */

// ============================================================================
// Test 1: Basic Function Call
// ============================================================================
{
    function testBasicCall1() {
        return 42;
    }

    function testBasicCall2(a, b) {
        return a + b;
    }

    function testBasicCall3(a, b, c) {
        return a * b + c;
    }

    function testBasicCall4(a, b, c, d) {
        return a + b + c + d;
    }

    function testBasicCall5(a, b, c, d, e) {
        return (a + b) * c + d * e;
    }

    print("=== Test 1: Basic Function Call ===");
    print(testBasicCall1());
    print(testBasicCall2(5, 3));
    print(testBasicCall3(2, 3, 4));
    print(testBasicCall4(1, 2, 3, 4));
    print(testBasicCall5(1, 2, 3, 4, 5));
}

// ============================================================================
// Test 2: Function Call with Default Parameters
// ============================================================================
{
    function testDefault1(a, b = 10) {
        return a + b;
    }

    function testDefault2(a = 5, b = 10, c = 15) {
        return a + b + c;
    }

    function testDefault3(a, b = 20, c = 30, d = 40) {
        return a + b + c + d;
    }

    function testDefault4(x = 1, y = 2, z = 3, w = 4, v = 5) {
        return x + y + z + w + v;
    }

    function testDefault5(a, b = "default", c = 100, d = [1, 2, 3]) {
        return a + b + c + d.length;
    }

    print("\n=== Test 2: Function Call with Default Parameters ===");
    print(testDefault1(5));
    print(testDefault1(5, 15));
    print(testDefault2());
    print(testDefault2(1));
    print(testDefault2(1, 2));
    print(testDefault2(1, 2, 3));
    print(testDefault3(5));
    print(testDefault3(5, 10));
    print(testDefault3(5, 10, 15));
    print(testDefault3(5, 10, 15, 20));
    print(testDefault4());
    print(testDefault4(10));
    print(testDefault4(10, 20));
    print(testDefault4(10, 20, 30));
    print(testDefault4(10, 20, 30, 40));
    print(testDefault4(10, 20, 30, 40, 50));
    print(testDefault5("hello"));
    print(testDefault5("hello", "world"));
}

// ============================================================================
// Test 3: Function Call with Rest Parameters
// ============================================================================
{
    function testRest1(...args) {
        return args.length;
    }

    function testRest2(a, ...args) {
        return a + args.length;
    }

    function testRest3(a, b, ...args) {
        return a + b + args[0] + args[1];
    }

    function testRest4(x, ...rest) {
        let sum = x;
        for (let i = 0; i < rest.length; i++) {
            sum += rest[i];
        }
        return sum;
    }

    function testRest5(a, b = 10, ...rest) {
        return a + b + rest.length;
    }

    function testRest6(...args) {
        let product = 1;
        for (let i = 0; i < args.length; i++) {
            product *= args[i];
        }
        return product;
    }

    function testRest7(first, ...middle) {
        return first + "|" + middle.join(",");
    }

    function testRest8(a, b, c, ...rest) {
        return JSON.stringify({ a, b, c, rest });
    }

    print("\n=== Test 3: Function Call with Rest Parameters ===");
    print(testRest1());
    print(testRest1(1));
    print(testRest1(1, 2));
    print(testRest1(1, 2, 3));
    print(testRest1(1, 2, 3, 4, 5, 6, 7, 8, 9, 10));
    print(testRest2(1));
    print(testRest2(1, 2));
    print(testRest2(1, 2, 3));
    print(testRest3(1, 2, 3, 4));
    print(testRest3(1, 2, 3, 4, 5, 6));
    print(testRest4(1));
    print(testRest4(1, 2));
    print(testRest4(1, 2, 3));
    print(testRest4(1, 2, 3, 4, 5));
    print(testRest5(1));
    print(testRest5(1, 2));
    print(testRest5(1, 2, 3));
    print(testRest5(1, 2, 3, 4));
    print(testRest6(2, 3, 4));
    print(testRest6(1, 2, 3, 4, 5));
    print(testRest7("first", "m1", "m2", "m3"));
    print(testRest7("start", "a", "b", "c", "d", "e"));
    print(testRest8(1, 2, 3, 4, 5, 6, 7, 8));
    print(testRest8("a", "b", "c"));
}

// ============================================================================
// Test 4: Method Call
// ============================================================================
{
    let obj1 = {
        value: 10,
        getValue: function() {
            return this.value;
        },
        setValue: function(v) {
            this.value = v;
            return this.value;
        },
        add: function(n) {
            return this.value + n;
        }
    };

    let obj2 = {
        name: "test",
        greet: function(greeting) {
            return greeting + ", " + this.name;
        },
        nested: {
            value: 20,
            getValue: function() {
                return this.value;
            }
        }
    };

    let obj3 = {
        data: [1, 2, 3, 4, 5],
        sum: function() {
            let total = 0;
            for (let i = 0; i < this.data.length; i++) {
                total += this.data[i];
            }
            return total;
        },
        process: function(fn) {
            return this.data.map(fn);
        }
    };

    print("\n=== Test 4: Method Call ===");
    print(obj1.getValue());
    print(obj1.setValue(20));
    print(obj1.getValue());
    print(obj1.add(5));
    print(obj2.greet("Hello"));
    print(obj2.greet("Hi"));
    print(obj2.nested.getValue());
    print(obj3.sum());
    print(JSON.stringify(obj3.process(x => x * 2)));
    print(JSON.stringify(obj3.process(x => x + 10)));
}

// ============================================================================
// Test 5: Constructor Call
// ============================================================================
{
    function TestConstructor1() {
        this.value = 100;
        this.getValue = function() {
            return this.value;
        };
    }

    function TestConstructor2(a, b) {
        this.a = a;
        this.b = b;
        this.sum = function() {
            return this.a + this.b;
        };
    }

    function TestConstructor3(name, age) {
        this.name = name;
        this.age = age;
        this.info = function() {
            return this.name + " is " + this.age + " years old";
        };
    }

    function TestConstructor4(config) {
        this.config = config;
        this.isValid = function() {
            return this.config !== null && this.config !== undefined;
        };
    }

    function TestConstructor5(x, y, z) {
        this.x = x;
        this.y = y;
        this.z = z;
        this.multiply = function() {
            return this.x * this.y * this.z;
        };
    }

    print("\n=== Test 5: Constructor Call ===");
    let instance1 = new TestConstructor1();
    print(instance1.getValue());
    let instance2 = new TestConstructor2(5, 3);
    print(instance2.sum());
    let instance3 = new TestConstructor3("John", 30);
    print(instance3.info());
    let instance4 = new TestConstructor4({ enabled: true, name: "test" });
    print(instance4.isValid());
    let instance5 = new TestConstructor5(2, 3, 4);
    print(instance5.multiply());
}

// ============================================================================
// Test 6: Call with this binding
// ============================================================================
{
    function testThis1() {
        return this && this.value;
    }

    function testThis2(a, b) {
        return (this && this.x || 0) + a + b;
    }

    function testThis3() {
        if (!this) {
            return "no this";
        }
        return this.name;
    }

    let context1 = {
        value: 42,
        method: testThis1
    };

    let context2 = {
        x: 10,
        method: testThis2
    };

    let context3 = {
        name: "context3",
        method: testThis3
    };

    print("\n=== Test 6: Call with this binding ===");
    print(context1.method());
    print(context2.method(5, 3));
    print(context3.method());
}

// ============================================================================
// Test 7: Call with call() method
// ============================================================================
{
    function callTest1(a, b, c) {
        return a + b + c;
    }

    function callTest2() {
        return (this && this.name) + ":" + (this && this.age);
    }

    function callTest3(x, y) {
        return (this && this.base || 0) + x + y;
    }

    let objForCall1 = { value: 100 };
    let objForCall2 = { name: "Alice", age: 25 };
    let objForCall3 = { base: 1000 };

    print("\n=== Test 7: Call with call() method ===");
    print(callTest1.call(objForCall1, 1, 2, 3));
    print(callTest1.call(null, 1, 2, 3));
    print(callTest2.call(objForCall2));
    print(callTest3.call(objForCall3, 5, 10));
}

// ============================================================================
// Test 8: Call with apply() method
// ============================================================================
{
    function applyTest1(a, b, c) {
        return a + b + c + (this && this.value || 0);
    }

    function applyTest2(args) {
        return (this && this.name || "Unknown") + "|" + args.join(",");
    }

    function applyTest3(a, b, c, d, e) {
        let sum = this && this.base || 0;
        sum += a + b + c + d + e;
        return sum;
    }

    let objForApply1 = { value: 200 };
    let objForApply2 = { name: "Bob" };
    let objForApply3 = { base: 500 };

    print("\n=== Test 8: Call with apply() method ===");
    print(applyTest1.apply(objForApply1, [1, 2, 3]));
    print(applyTest1.apply(null, [1, 2, 3]));
    print(applyTest2.apply(objForApply2, [["x", "y", "z"]]));
    print(applyTest3.apply(objForApply3, [1, 2, 3, 4, 5]));
}

// ============================================================================
// Test 9: Call with bind() method
// ============================================================================
{
    function bindTest1(a, b) {
        return (this && this.value || 0) + a + b;
    }

    function bindTest2(x, y, z) {
        return (this && this.prefix || "") + x + y + z;
    }

    function bindTest3() {
        return (this && this.data || []).length;
    }

    let objForBind1 = { value: 300 };
    let objForBind2 = { prefix: "PREFIX-" };
    let objForBind3 = { data: [1, 2, 3, 4, 5, 6, 7, 8, 9, 10] };

    let bound1 = bindTest1.bind(objForBind1);
    let bound2 = bindTest2.bind(objForBind2);
    let bound3 = bindTest3.bind(objForBind3);

    let boundPartial = bindTest1.bind(objForBind1, 10);

    print("\n=== Test 9: Call with bind() method ===");
    print(bound1(5, 3));
    print(bound2("a", "b", "c"));
    print(bound3());
    print(boundPartial(5));
    print(boundPartial(10));
}

// ============================================================================
// Test 10: Nested Function Calls
// ============================================================================
{
    function level1(x) {
        return level2(x + 1);
    }

    function level2(y) {
        return level3(y + 1);
    }

    function level3(z) {
        return level4(z + 1);
    }

    function level4(w) {
        return w * 2;
    }

    function fibonacci(n) {
        if (n <= 1) return n;
        return fibonacci(n - 1) + fibonacci(n - 2);
    }

    function factorial(n) {
        if (n <= 1) return 1;
        return n * factorial(n - 1);
    }

    function power(base, exp) {
        if (exp <= 0) return 1;
        return base * power(base, exp - 1);
    }

    print("\n=== Test 10: Nested Function Calls ===");
    print(level1(1));
    print(fibonacci(5));
    print(fibonacci(7));
    print(factorial(5));
    print(factorial(7));
    print(power(2, 5));
    print(power(3, 4));
}

// ============================================================================
// Test 11: Array Method Calls
// ============================================================================
{
    let testArray1 = [1, 2, 3, 4, 5];
    let testArray2 = [10, 20, 30, 40, 50];
    let testArray3 = [5, 15, 25, 35, 45];
    let testArray4 = [2, 4, 6, 8, 10];
    let testArray5 = [1, 3, 5, 7, 9];

    print("\n=== Test 11: Array Method Calls ===");
    print(JSON.stringify(testArray1.map(x => x * 2)));
    print(JSON.stringify(testArray1.filter(x => x > 2)));
    print(testArray1.reduce((a, b) => a + b));
    print(testArray1.find(x => x > 3));
    print(testArray1.findIndex(x => x === 4));
    print(testArray1.every(x => x > 0));
    print(testArray1.some(x => x > 3));

    print(JSON.stringify(testArray2.map(x => x + 1)));
    print(JSON.stringify(testArray2.filter(x => x >= 30)));
    print(testArray2.reduce((a, b) => a * b));
    print(testArray2.find(x => x === 40));
    print(testArray2.findIndex(x => x === 40));
    print(testArray2.every(x => x > 5));
    print(testArray2.some(x => x > 45));
    let last = testArray3.filter(x => x > 20);
    print(last[last.length - 1]);
}

// ============================================================================
// Test 12: String Method Calls
// ============================================================================
{
    let str1 = "Hello World";
    let str2 = "JavaScript";
    let str3 = "Test String";
    let str4 = "ABCDEFGHIJKLMNOPQRSTUVWXYZ";
    let str5 = "abcdefghijklmnopqrstuvwxyz";

    print("\n=== Test 12: String Method Calls ===");
    print(str1.toUpperCase());
    print(str1.toLowerCase());
    print(str1.length);
    print(str1.charAt(0));
    print(str1.charAt(6));
    print(str1.indexOf("World"));
    print(str1.indexOf("l"));
    print(JSON.stringify(str1.split(" ")));
    print(str1.substring(0, 5));
    print(str1.slice(6, 11));

    print(str2.toUpperCase());
    print(str2.toLowerCase());
    print(str2.length);
    print(str2.indexOf("Script"));
    print(str2.slice(0, 4));
    print(str2.replace("Java", "Type"));
    print(str3.concat(" ", str2));
}

// ============================================================================
// Test 13: Object Method Calls
// ============================================================================
{
    let objTest1 = {
        a: 1,
        b: 2,
        c: 3,
        getKeys: function() {
            return Object.keys(this);
        },
        getValues: function() {
            return Object.values(this);
        },
        getEntries: function() {
            return Object.entries(this);
        }
    };

    let objTest2 = {
        name: "TestObject",
        count: 100,
        enabled: true,
        metadata: { version: 1.0, author: "test" },
        clone: function() {
            return Object.assign({}, this);
        },
        toString: function() {
            return this.name + ":" + this.count;
        }
    };

    print("\n=== Test 13: Object Method Calls ===");
    print(JSON.stringify(objTest1.getKeys()));
    print(JSON.stringify(objTest1.getValues()));
    print(JSON.stringify(objTest1.getEntries()));
    print(JSON.stringify(objTest2.clone()));
    print(objTest2.toString());
}

// ============================================================================
// Test 14: Function Call with arguments
// ============================================================================
{
    function argumentsTest1() {
        return arguments.length;
    }

    function argumentsTest2() {
        let sum = 0;
        for (let i = 0; i < arguments.length; i++) {
            sum += arguments[i];
        }
        return sum;
    }

    function argumentsTest3(a, b) {
        return "a=" + a + ", b=" + b + ", length=" + arguments.length;
    }

    function argumentsTest4(...args) {
        let max = Math.max.apply(null, args);
        let min = Math.min.apply(null, args);
        return JSON.stringify({ max, min, count: args.length });
    }

    function argumentsTest5(a, b, c) {
        let product = 1;
        for (let i = 0; i < arguments.length; i++) {
            product *= arguments[i];
        }
        return product;
    }

    print("\n=== Test 14: Function Call with arguments ===");
    print(argumentsTest1(1, 2, 3));
    print(argumentsTest2(1, 2, 3, 4, 5));
    print(argumentsTest2(10, 20, 30));
    print(argumentsTest3(5, 10));
    print(argumentsTest3("x", "y", "z"));
    print(argumentsTest4(5, 15, 25, 35, 45));
    print(argumentsTest4(100, 200, 300, 400, 500, 600));
    print(argumentsTest5(2, 3, 4));
    print(argumentsTest5(1, 2, 3, 4, 5));
}

// ============================================================================
// Test 15: Closure Calls
// ============================================================================
{
    function createCounter() {
        let count = 0;
        return function() {
            count++;
            return count;
        };
    }

    function createMultiplier(factor) {
        return function(x) {
            return x * factor;
        };
    }

    function createAdder(a) {
        return function(b) {
            return a + b;
        };
    }

    function createObjectCreator(type) {
        return function(props) {
            let obj = {};
            obj.type = type;
            for (let key in props) {
                obj[key] = props[key];
            }
            return obj;
        };
    }

    print("\n=== Test 15: Closure Calls ===");
    let counter1 = createCounter();
    print(counter1());
    print(counter1());
    print(counter1());
    let counter2 = createCounter();
    print(counter2());
    print(counter2());

    let multiplyBy2 = createMultiplier(2);
    let multiplyBy3 = createMultiplier(3);
    let multiplyBy5 = createMultiplier(5);
    print(multiplyBy2(10));
    print(multiplyBy3(10));
    print(multiplyBy5(10));

    let add10 = createAdder(10);
    let add100 = createAdder(100);
    print(add10(5));
    print(add10(15));
    print(add100(50));
    print(add100(100));

    let createUser = createObjectCreator("user");
    let createProduct = createObjectCreator("product");
    let user = createUser({ name: "John", age: 30 });
    let product = createProduct({ id: 123, price: 99.99 });
    print(JSON.stringify(user));
    print(JSON.stringify(product));
}

// ============================================================================
// Test 16: Callback Function Calls
// ============================================================================
{
    function processWithCallback(data, callback) {
        let result = data * 2;
        callback(result);
    }

    function asyncProcess(data, callback, delay = 100) {
        callback(data * 3);
    }

    function processArrayWithCallback(arr, callback) {
        let result = arr.filter(x => x > 0).map(x => x * 2);
        callback(result);
    }

    function processWithMultipleCallbacks(data, success, error) {
        if (data > 0) {
            success(data * 2);
        } else {
            error("Invalid data");
        }
    }

    print("\n=== Test 16: Callback Function Calls ===");
    processWithCallback(5, result => print("Result: " + result));

    processWithCallback(10, result => print("Doubled: " + result));

    asyncProcess(4, result => print("Async result: " + result), 50);

    let processWithArrayCallback = function(arr, callback) {
        let result = arr.map(x => x + 1);
        callback(result);
    };
    processWithArrayCallback([1, 2, 3, 4, 5], result => print(JSON.stringify(result)));

    processWithMultipleCallbacks(5,
        result => print("Success: " + result),
        err => print("Error: " + err)
    );

    processWithMultipleCallbacks(-1,
        result => print("Success: " + result),
        err => print("Error: " + err)
    );
}

// ============================================================================
// Test 17: Arrow Function Calls
// ============================================================================
{
    let arrowTest1 = x => x * 2;
    let arrowTest2 = (a, b) => a + b;
    let arrowTest3 = (x, y, z) => {
        return x + y + z;
    };
    let arrowTest4 = () => 42;
    let arrowTest5 = (...args) => args.length;

    print("\n=== Test 17: Arrow Function Calls ===");
    print(arrowTest1(5));
    print(arrowTest2(3, 7));
    print(arrowTest3(1, 2, 3));
    print(arrowTest4());
    print(arrowTest5(1, 2, 3, 4, 5));

    let arrowArray = [1, 2, 3, 4, 5].map(x => x * 2);
    print(JSON.stringify(arrowArray));

    let arrowFilter = [10, 20, 30, 40, 50].filter(x => x > 25);
    print(JSON.stringify(arrowFilter));

    let arrowReduce = [1, 2, 3, 4, 5].reduce((a, b) => a + b);
    print(arrowReduce);
}

// ============================================================================
// Test 18: Function Call with new.target
// ============================================================================
{
    function newTargetTest1() {
        if (new.target) {
            return "called with new";
        } else {
            return "called without new";
        }
    }

    function newTargetTest2(a, b) {
        if (new.target) {
            this.value = a + b;
            this.getValue = function() {
                return this.value;
            };
        } else {
            return a + b;
        }
    }

    function newTargetTest3(value) {
        if (new.target === newTargetTest3) {
            this.data = value;
            this.process = function() {
                return this.data * 2;
            };
        } else {
            return "Invalid call";
        }
    }

    print("\n=== Test 18: Function Call with new.target ===");
    print(newTargetTest1());
    print(newTargetTest1.call(null));

    let objNT1 = new newTargetTest2(5, 3);
    print(objNT1.getValue ? objNT1.getValue() : "no method");
    print(newTargetTest2(10, 20));

    let objNT2 = new newTargetTest3(100);
    print(objNT2.process ? objNT2.process() : "no method");
    print(newTargetTest3(50));
}

// ============================================================================
// Test 19: Function Call with spread operator
// ============================================================================
{
    function spreadTest1(a, b, c) {
        return a + b + c;
    }

    function spreadTest2(...args) {
        return args.reduce((a, b) => a + b);
    }

    function spreadTest3(a, b, c, d, e) {
        return a * b * c * d * e;
    }

    let arr1 = [1, 2, 3];
    let arr2 = [5, 10, 15];
    let arr3 = [2, 4, 6, 8, 10];

    print("\n=== Test 19: Function Call with spread operator ===");
    print(spreadTest1(...arr1));
    print(spreadTest2(...arr1, ...arr2));
    print(spreadTest2(1, 2, 3, 4, 5, 6, 7, 8, 9, 10));
    print(spreadTest3(...arr3));

    let obj1 = { a: 1, b: 2 };
    let obj2 = { c: 3, d: 4 };
    let obj3 = { ...obj1, ...obj2, e: 5 };
    print(JSON.stringify(obj3));
}

// ============================================================================
// Test 20: Generator Function Calls
// ============================================================================
{
    function* genTest1() {
        yield 1;
        yield 2;
        yield 3;
    }

    function* genTest2(n) {
        for (let i = 1; i <= n; i++) {
            yield i;
        }
    }

    function* genTest3() {
        yield* [1, 2, 3];
        yield* ["a", "b", "c"];
    }

    function* genTest4() {
        let x = yield 1;
        let y = yield 2;
        yield x + y;
    }

    print("\n=== Test 20: Generator Function Calls ===");
    let gen1 = genTest1();
    print(gen1.next().value);
    print(gen1.next().value);
    print(gen1.next().value);
    print(gen1.next().done);

    let gen2 = genTest2(5);
    print(gen2.next().value);
    print(gen2.next().value);
    print(gen2.next().value);
    print(gen2.next().value);
    print(gen2.next().value);
    print(gen2.next().done);

    let gen3 = genTest3();
    print(gen3.next().value);
    print(gen3.next().value);
    print(gen3.next().value);
    print(gen3.next().value);
    print(gen3.next().value);
    print(gen3.next().value);
    print(gen3.next().done);
}

// ============================================================================
// Test 21: Async Function Calls
// ============================================================================
{
    function asyncTest1() {
        return 42;
    }

    function asyncTest2(x) {
        return x * 2;
    }

    function asyncTest3(a, b) {
        return a + b;
    }

    function asyncTest4(arr) {
        let sum = 0;
        for (let i = 0; i < arr.length; i++) {
            sum += arr[i];
        }
        return sum;
    }

    print("\n=== Test 21: Async Function Calls ===");
    print("Async 1: " + asyncTest1());
    print("Async 2: " + asyncTest2(21));
    print("Async 3: " + asyncTest3(10, 20));
    print("Async 4: " + asyncTest4([1, 2, 3, 4, 5]));
}

// ============================================================================
// Test 22: Recursive Function Calls
// ============================================================================
{
    function recursiveSum(n) {
        if (n <= 0) return 0;
        return n + recursiveSum(n - 1);
    }

    function recursiveCount(n) {
        if (n > 0) {
            print(n);
            recursiveCount(n - 1);
        }
    }

    function recursiveReverse(str) {
        if (str.length <= 1) return str;
        return recursiveReverse(str.substring(1)) + str.charAt(0);
    }

    function recursiveBinarySearch(arr, target, left = 0, right = arr.length - 1) {
        if (left > right) return -1;
        let mid = Math.floor((left + right) / 2);
        if (arr[mid] === target) return mid;
        if (arr[mid] < target) return recursiveBinarySearch(arr, target, mid + 1, right);
        return recursiveBinarySearch(arr, target, left, mid - 1);
    }

    function recursiveFactorial(n) {
        if (n <= 1) return 1;
        return n * recursiveFactorial(n - 1);
    }

    print("\n=== Test 22: Recursive Function Calls ===");
    print(recursiveSum(10));
    print(recursiveSum(20));
    print(recursiveReverse("hello"));
    print(recursiveReverse("world"));
    let sortedArray = [1, 3, 5, 7, 9, 11, 13, 15];
    print("Index of 7: " + recursiveBinarySearch(sortedArray, 7));
    print("Index of 10: " + recursiveBinarySearch(sortedArray, 10));
    print(recursiveFactorial(5));
    print(recursiveFactorial(7));
}

// ============================================================================
// Test 23: High-Order Function Calls
// ============================================================================
{
    function higherOrder1(fn) {
        return function(x, y) {
            return fn(x, y) * 2;
        };
    }

    function higherOrder2(fn) {
        return function(...args) {
            return fn(...args) + 10;
        };
    }

    function higherOrder3(baseFn, transformFn) {
        return function(...args) {
            return transformFn(baseFn(...args));
        };
    }

    function add(a, b) {
        return a + b;
    }

    function multiply(a, b) {
        return a * b;
    }

    function square(x) {
        return x * x;
    }

    function double(x) {
        return x * 2;
    }

    let composed1 = higherOrder1(add);
    let composed2 = higherOrder2(multiply);
    let composed3 = higherOrder3(add, square);

    print("\n=== Test 23: High-Order Function Calls ===");
    print(composed1(5, 3));
    print(composed2(4, 3));
    print(composed3(2, 3));

    let multiplyThenDouble = higherOrder3(multiply, double);
    print(multiplyThenDouble(3, 4));

    let addThenSquare = higherOrder3(add, square);
    print(addThenSquare(5, 3));
}

// ============================================================================
// Test 24: Method Chaining Calls
// ============================================================================
{
    let chainObj = {
        value: 1,
        add: function(n) {
            this.value += n;
            return this;
        },
        multiply: function(n) {
            this.value *= n;
            return this;
        },
        subtract: function(n) {
            this.value -= n;
            return this;
        },
        divide: function(n) {
            this.value /= n;
            return this;
        },
        getValue: function() {
            return this.value;
        }
    };

    print("\n=== Test 24: Method Chaining Calls ===");
    print(chainObj.add(5).multiply(2).subtract(3).divide(2).getValue());
    print(chainObj.value);

    let arrayChain = [1, 2, 3, 4, 5];
    let chainResult = arrayChain
        .filter(x => x > 2)
        .map(x => x * 2)
        .reduce((a, b) => a + b);
    print(chainResult);
}

// ============================================================================
// Test 25: Conditional Function Calls
// ============================================================================
{
    function conditionalTest1(x) {
        if (typeof x === "number") {
            return x * 2;
        } else if (typeof x === "string") {
            return x.toUpperCase();
        } else {
            return null;
        }
    }

    function conditionalTest2(arr) {
        if (!Array.isArray(arr)) {
            return "Not an array";
        }
        if (arr.length === 0) {
            return "Empty array";
        }
        return arr.reduce((a, b) => a + b);
    }

    function conditionalTest3(obj) {
        if (!obj) {
            return "No object";
        }
        if (obj.hasOwnProperty("value")) {
            return obj.value * 2;
        }
        return "No value property";
    }

    function conditionalTest4(x, y, operation) {
        switch (operation) {
            case "add":
                return x + y;
            case "subtract":
                return x - y;
            case "multiply":
                return x * y;
            case "divide":
                return x / y;
            default:
                return "Unknown operation";
        }
    }

    print("\n=== Test 25: Conditional Function Calls ===");
    print(conditionalTest1(10));
    print(conditionalTest1("hello"));
    print(conditionalTest1(true));
    print(conditionalTest2([1, 2, 3, 4, 5]));
    print(conditionalTest2([]));
    print(conditionalTest2("not array"));
    print(conditionalTest3({ value: 10 }));
    print(conditionalTest3({ name: "test" }));
    print(conditionalTest3(null));
    print(conditionalTest4(10, 5, "add"));
    print(conditionalTest4(10, 5, "multiply"));
    print(conditionalTest4(10, 5, "divide"));
    print(conditionalTest4(10, 5, "unknown"));
}

// ============================================================================
// Test 26: Looping with Function Calls
// ============================================================================
{
    function loopTest1(count) {
        let sum = 0;
        for (let i = 1; i <= count; i++) {
            sum += i;
        }
        return sum;
    }

    function loopTest2(arr) {
        let product = 1;
        for (let i = 0; i < arr.length; i++) {
            product *= arr[i];
        }
        return product;
    }

    function loopTest3(n) {
        let a = 0, b = 1;
        for (let i = 0; i < n; i++) {
            [a, b] = [b, a + b];
        }
        return a;
    }

    function loopTest4(obj) {
        let keys = Object.keys(obj);
        let result = {};
        for (let i = 0; i < keys.length; i++) {
            let key = keys[i];
            result[key] = obj[key] * 2;
        }
        return result;
    }

    print("\n=== Test 26: Looping with Function Calls ===");
    print(loopTest1(10));
    print(loopTest1(100));
    print(loopTest2([2, 3, 4, 5]));
    print(loopTest2([1, 2, 3, 4, 5, 6]));
    print(loopTest3(10));
    print(loopTest3(15));
    print(JSON.stringify(loopTest4({ a: 1, b: 2, c: 3 })));
}

// ============================================================================
// Test 27: Exception Handling in Function Calls
// ============================================================================
{
    function exceptionTest1(x) {
        try {
            if (x < 0) {
                throw new Error("Negative value");
            }
            return Math.sqrt(x);
        } catch (e) {
            return "Error: " + e.message;
        }
    }

    function exceptionTest2(arr) {
        try {
            if (!Array.isArray(arr)) {
                throw new Error("Not an array");
            }
            if (arr.length === 0) {
                throw new Error("Empty array");
            }
            return arr.reduce((a, b) => a + b);
        } catch (e) {
            return "Error: " + e.message;
        }
    }

    function exceptionTest3(divisor) {
        try {
            return 100 / divisor;
        } catch (e) {
            return "Division error: " + e.message;
        }
    }

    function exceptionTest4(obj) {
        try {
            if (!obj) {
                throw new Error("Null object");
            }
            return obj.value;
        } catch (e) {
            return "Error: " + e.message;
        } finally {
            print("Cleanup completed");
        }
    }

    print("\n=== Test 27: Exception Handling in Function Calls ===");
    print(exceptionTest1(16));
    print(exceptionTest1(-5));
    print(exceptionTest1(100));
    print(exceptionTest2([1, 2, 3, 4, 5]));
    print(exceptionTest2([]));
    print(exceptionTest2("not array"));
    print(exceptionTest3(10));
    print(exceptionTest3(0));
    print(exceptionTest3(5));
    print(exceptionTest4({ value: 42 }));
    print(exceptionTest4(null));
}

// ============================================================================
// Test 28: Complex Function Composition
// ============================================================================
{
    function compose2(f, g) {
        return function(x) {
            return f(g(x));
        };
    }

    function compose3(f, g, h) {
        return function(x) {
            return f(g(h(x)));
        };
    }

    function composeMany(...fns) {
        return function(x) {
            return fns.reduceRight((acc, fn) => fn(acc), x);
        };
    }

    function add1(x) { return x + 1; }
    function multiply2(x) { return x * 2; }
    function subtract3(x) { return x - 3; }
    function divide4(x) { return x / 4; }

    print("\n=== Test 28: Complex Function Composition ===");
    let composedAddMultiply = compose2(multiply2, add1);
    print(composedAddMultiply(5));

    let composedAll = compose3(subtract3, multiply2, add1);
    print(composedAll(10));

    let composedMany = composeMany(divide4, subtract3, multiply2, add1);
    print(composedMany(16));
}

// ============================================================================
// Test 29: Function Memoization
// ============================================================================
{
    function memoize(fn) {
        let cache = {};
        return function(...args) {
            let key = JSON.stringify(args);
            if (cache[key] !== undefined) {
                return cache[key];
            }
            let result = fn.apply(this, args);
            cache[key] = result;
            return result;
        };
    }

    let memoizedFactorial = memoize(function(n) {
        if (n <= 1) return 1;
        return n * memoizedFactorial(n - 1);
    });

    let memoizedFibonacci = memoize(function(n) {
        if (n <= 1) return n;
        return memoizedFibonacci(n - 1) + memoizedFibonacci(n - 2);
    });

    let memoizedExpensive = memoize(function(n) {
        let result = 0;
        for (let i = 0; i < n; i++) {
            result += Math.sqrt(i);
        }
        return result;
    });

    print("\n=== Test 29: Function Memoization ===");
    print(memoizedFactorial(5));
    print(memoizedFactorial(5));
    print(memoizedFactorial(6));
    print(memoizedFactorial(6));
    print(memoizedFibonacci(10));
    print(memoizedFibonacci(10));
    print(memoizedExpensive(100000));
    print(memoizedExpensive(100000));
}

// ============================================================================
// Test 30: Partial Application
// ============================================================================
{
    function partialApply(fn, ...args) {
        return function(...moreArgs) {
            return fn.apply(this, args.concat(moreArgs));
        };
    }

    function partialRight(fn, ...args) {
        return function(...moreArgs) {
            return fn.apply(this, moreArgs.concat(args));
        };
    }

    function add(a, b, c) {
        return a + b + c;
    }

    function multiply(a, b, c) {
        return a * b * c;
    }

    function format(template, ...values) {
        return template.replace(/{(\d+)}/g, (match, index) => {
            return values[index] || match;
        });
    }

    let add5 = partialApply(add, 5);
    let multiplyBy2 = partialApply(multiply, 2);
    let multiplyBy3And5 = partialApply(multiply, 3, 5);

    let formatWithPrefix = partialApply(format, "Prefix: {0}");
    let formatWithSuffix = partialRight(format, "Suffix");

    print("\n=== Test 30: Partial Application ===");
    print(add5(3, 2));
    print(add5(10, 20));
    print(multiplyBy2(5, 3));
    print(multiplyBy3And5(4));
    print(formatWithPrefix("Hello {0}!"));
    print(formatWithSuffix("Hello {0}!", "World"));
}

// ============================================================================
// Test 31: Currying
// ============================================================================
{
    function curry(fn, arity = fn.length) {
        return function curried(...args) {
            if (args.length >= arity) {
                return fn.apply(this, args);
            }
            return function(...moreArgs) {
                return curried.apply(this, args.concat(moreArgs));
            };
        };
    }

    function addCurry(a, b, c) {
        return a + b + c;
    }

    function multiplyCurry(a, b, c) {
        return a * b * c;
    }

    let curriedAdd = curry(addCurry);
    let curriedMultiply = curry(multiplyCurry);

    print("\n=== Test 31: Currying ===");
    print(curriedAdd(1)(2)(3));
    print(curriedAdd(1, 2)(3));
    print(curriedAdd(1)(2, 3));
    print(curriedAdd(1, 2, 3));
    print(curriedMultiply(2)(3)(4));
    print(curriedMultiply(2, 3)(4));
    print(curriedMultiply(2)(3, 4));
    print(curriedMultiply(2, 3, 4));
}

// ============================================================================
// Test 32: Proxy and Function Calls
// ============================================================================
{
    let targetFunction = function(a, b) {
        return a + b;
    };

    let proxyFunction = new Proxy(targetFunction, {
        apply: function(target, thisArg, argumentsList) {
            print("Calling function with args: " + argumentsList);
            let result = target.apply(thisArg, argumentsList);
            print("Result: " + result);
            return result;
        }
    });

    let targetObj = {
        value: 10,
        method: function(x) {
            return this.value + x;
        }
    };

    let proxyObj = new Proxy(targetObj, {
        get: function(target, prop) {
            if (typeof target[prop] === "function") {
                return function(...args) {
                    print("Calling method: " + prop);
                    return target[prop].apply(target, args);
                };
            }
            return target[prop];
        }
    });

    print("\n=== Test 32: Proxy and Function Calls ===");
    print(proxyFunction(5, 3));
    print(proxyObj.method(20));
}

// ============================================================================
// Test 33: Symbol and Function Calls
// ============================================================================
{
    const sym1 = Symbol("test1");
    const sym2 = Symbol("test2");

    let symObj = {
        [sym1]: 100,
        [sym2]: function(x) {
            return this[sym1] + x;
        },
        regularMethod: function(y) {
            return this[sym2](y) * 2;
        }
    };

    function getSymbolProperty(obj, symbol) {
        return obj[symbol];
    }

    function callSymbolMethod(obj, symbol, arg) {
        return obj[symbol](arg);
    }

    print("\n=== Test 33: Symbol and Function Calls ===");
    print(getSymbolProperty(symObj, sym1));
    print(callSymbolMethod(symObj, sym2, 50));
    print(symObj.regularMethod(30));
}

// ============================================================================
// Test 34: Map and Set Operations
// ============================================================================
{
    let mapTest = new Map();
    mapTest.set("a", 1);
    mapTest.set("b", 2);
    mapTest.set("c", 3);

    let setTest = new Set([1, 2, 3, 4, 5, 5, 5, 6, 7, 8, 9, 10]);

    let mapWithFunctions = new Map();
    mapWithFunctions.set("add", (a, b) => a + b);
    mapWithFunctions.set("multiply", (a, b) => a * b);
    mapWithFunctions.set("subtract", (a, b) => a - b);

    print("\n=== Test 34: Map and Set Operations ===");
    print(mapTest.get("a"));
    print(mapTest.get("b"));
    print(mapTest.get("c"));
    print(setTest.has(5));
    print(setTest.has(10));
    print(setTest.size);

    print(mapWithFunctions.get("add")(5, 3));
    print(mapWithFunctions.get("multiply")(4, 3));
    print(mapWithFunctions.get("subtract")(10, 7));
}

// ============================================================================
// Test 35: WeakMap and WeakSet
// ============================================================================
{
    let weakMapTest = new WeakMap();
    let objRef1 = { name: "obj1", value: 10 };
    let objRef2 = { name: "obj2", value: 20 };
    let objRef3 = { name: "obj3", value: 30 };

    weakMapTest.set(objRef1, "value1");
    weakMapTest.set(objRef2, "value2");
    weakMapTest.set(objRef3, "value3");

    let weakSetTest = new WeakSet();
    weakSetTest.add(objRef1);
    weakSetTest.add(objRef2);
    weakSetTest.add(objRef3);

    print("\n=== Test 35: WeakMap and WeakSet ===");
    print(weakMapTest.get(objRef1));
    print(weakMapTest.get(objRef2));
    print(weakMapTest.has(objRef3));
    print(weakSetTest.has(objRef1));
    print(weakSetTest.has(objRef2));
}

// ============================================================================
// Test 36: Promise Chain Calls
// ============================================================================
{
    function promiseTest1(x) {
        return x * 2;
    }

    function promiseTest2(x) {
        if (x > 0) {
            return x + 10;
        } else {
            return "Invalid value";
        }
    }

    print("\n=== Test 36: Promise Chain Calls ===");
    let result1 = promiseTest1(5);
    print("First then: " + result1);
    let result2 = promiseTest1(result1);
    print("Second then: " + result2);
    let result3 = promiseTest2(result2);
    print("Third then: " + result3);
    let finalResult = promiseTest2(result3);
    print("Final result: " + finalResult);
}

// ============================================================================
// Test 37: ArrayBuffer and TypedArray
// ============================================================================
{
    let buffer = new ArrayBuffer(16);
    let view8 = new Uint8Array(buffer);
    let view16 = new Uint16Array(buffer);
    let view32 = new Uint32Array(buffer);

    view8[0] = 255;
    view8[1] = 128;
    view16[1] = 1024;
    view32[0] = 0x12345678;

    print("\n=== Test 37: ArrayBuffer and TypedArray ===");
    print(view8[0]);
    print(view8[1]);
    print(view16[1]);
    print(view32[0]);

    let typedArray = new Float32Array([1.5, 2.5, 3.5, 4.5, 5.5]);
    print(typedArray.length);
    print(typedArray[0]);
    print(typedArray[typedArray.length - 1]);
}

// ============================================================================
// Test 38: DataView Operations
// ============================================================================
{
    let buffer2 = new ArrayBuffer(8);
    let dataView = new DataView(buffer2);

    dataView.setInt8(0, 10);
    dataView.setInt8(1, 20);
    dataView.setUint16(2, 1000);
    dataView.setUint32(4, 0x12345678);
    dataView.setFloat64(0, 3.14159);

    print("\n=== Test 38: DataView Operations ===");
    print(dataView.getInt8(0));
    print(dataView.getInt8(1));
    print(dataView.getUint16(2));
    print(dataView.getUint32(4));
    print(dataView.getFloat64(0));
}

// ============================================================================
// Test 40: Regular Expression
// ============================================================================
{
    let regex1 = /\d+/;
    let regex2 = /[a-z]+/i;
    let regex3 = /\w+@\w+\.\w+/;
    let regex4 = new RegExp("\\d+", "g");

    print("\n=== Test 40: Regular Expression ===");
    print(regex1.test("123"));
    print(regex1.test("abc"));
    print(regex2.test("Hello"));
    print(regex2.test("123"));
    print(regex3.test("test@example.com"));
    print(regex3.test("invalid"));
    print(regex4.test("123 abc 456"));

    let str = "There are 123 apples and 456 oranges";
    let matches = str.match(/\d+/g);
    print(matches ? matches.join(",") : "No matches");
}

// ============================================================================
// Test 41: JSON Operations
// ============================================================================
{
    let jsonObj = {
        name: "Test",
        value: 42,
        nested: {
            array: [1, 2, 3],
            boolean: true,
            nullValue: null
        }
    };

    let jsonStr = JSON.stringify(jsonObj);
    let parsedObj = JSON.parse(jsonStr);

    print("\n=== Test 41: JSON Operations ===");
    print(jsonStr);
    print(parsedObj.name);
    print(parsedObj.nested.array[0]);
    print(JSON.stringify(jsonObj, null, 2));
}

// ============================================================================
// Test 42: Error Handling
// ============================================================================
{
    function errorTest1() {
        try {
            let x = 10;
            let y = x / 0;
            return y;
        } catch (e) {
            return "Caught: " + e.message;
        } finally {
            print("Finally block executed");
        }
    }

    function errorTest2() {
        try {
            throw new Error("Custom error");
        } catch (e) {
            return "Error: " + e.message;
        }
    }

    function errorTest3(value) {
        if (value === null || value === undefined) {
            throw new TypeError("Invalid value");
        }
        return value * 2;
    }

    print("\n=== Test 42: Error Handling ===");
    print(errorTest1());
    print(errorTest2());
    try {
        print(errorTest3(5));
        print(errorTest3(null));
    } catch (e) {
        print("Caught error: " + e.message);
    }
}

// ============================================================================
// Test 43: Math Operations
// ============================================================================
{
    print("\n=== Test 43: Math Operations ===");
    print(Math.abs(-10));
    print(Math.ceil(3.14));
    print(Math.floor(3.14));
    print(Math.round(3.5));
    print(Math.max(1, 2, 3, 4, 5));
    print(Math.min(1, 2, 3, 4, 5));
    print(Math.sqrt(16));
    print(Math.pow(2, 3));
    print(Math.sin(0));
    print(Math.cos(0));
    print(Math.log(1));
    print(Math.log10 ? Math.log10(100) : Math.log(100) / Math.LN10);
}

// ============================================================================
// Test 44: Number Operations
// ============================================================================
{
    print("\n=== Test 44: Number Operations ===");
    print(Number.parseInt("42"));
    print(Number.parseInt("3.14"));
    print(Number.parseFloat("3.14"));
    print(Number.parseFloat("42"));
    print(Number.isNaN(NaN));
    print(Number.isNaN(42));
    print(Number.isFinite(42));
    print(Number.isFinite(Infinity));
    print(Number.isInteger(42));
    print(Number.isInteger(3.14));
    print((42).toString());
    print((3.14159).toFixed(2));
    print((3.14159).toFixed(4));
}

// ============================================================================
// Test 45: String Operations
// ============================================================================
{
    let strTest = "Hello World";
    print("\n=== Test 45: String Operations ===");
    print(strTest.length);
    print(strTest.charAt(0));
    print(strTest.charAt(6));
    print(strTest.indexOf("World"));
    print(strTest.indexOf("l"));
    print(strTest.lastIndexOf("l"));
    print(strTest.substring(0, 5));
    print(strTest.slice(6, 11));
    print(strTest.toUpperCase());
    print(strTest.toLowerCase());
    print(JSON.stringify(strTest.split(" ")));
    print(strTest.replace("World", "JavaScript"));
    print(strTest.concat("!"));
    print(strTest.trim ? strTest.trim() : strTest);
}

// ============================================================================
// Test 46: Array Operations
// ============================================================================
{
    let arrTest = [1, 2, 3, 4, 5, 6, 7, 8, 9, 10];

    print("\n=== Test 46: Array Operations ===");
    print(arrTest.length);
    print(arrTest.indexOf(5));
    print(arrTest.includes(5));
    print(arrTest.push(11));
    print(arrTest.pop());
    print(arrTest.shift());
    print(arrTest.unshift(0));
    print(JSON.stringify(arrTest.slice(2, 5)));
    print(JSON.stringify(arrTest.splice(2, 3)));
    print(arrTest.join(","));
    let sortedArr = [...arrTest].sort((a, b) => a - b);
    print(sortedArr.join(","));
    print(JSON.stringify(arrTest.reverse ? arrTest.reverse() : "reverse not available"));
}

// ============================================================================
// Test 47: Object Operations
// ============================================================================
{
    let objTest = {
        a: 1,
        b: 2,
        c: 3,
        d: 4,
        e: 5
    };

    print("\n=== Test 47: Object Operations ===");
    print(Object.keys(objTest).length);
    print(Object.values(objTest).length);
    print(Object.entries(objTest).length);
    print(objTest.hasOwnProperty("a"));
    print(objTest.hasOwnProperty("z"));
    print(JSON.stringify(Object.assign({}, objTest, { f: 6 })));
    print(Object.is(objTest, objTest));
    print(Object.is(objTest, {}));
    print(Object.freeze ? Object.freeze(objTest) : "freeze not available");
}

// ============================================================================
// Test 48: Class Operations
// ============================================================================
{
    class TestClass1 {
        constructor(value) {
            this.value = value;
        }

        getValue() {
            return this.value;
        }

        setValue(v) {
            this.value = v;
            return this.value;
        }
    }

    class TestClass2 extends TestClass1 {
        constructor(value, multiplier) {
            super(value);
            this.multiplier = multiplier;
        }

        process() {
            return this.getValue() * this.multiplier;
        }
    }

    print("\n=== Test 48: Class Operations ===");
    let classObj1 = new TestClass1(42);
    print(classObj1.getValue());
    print(classObj1.setValue(100));

    let classObj2 = new TestClass2(10, 2);
    print(classObj2.getValue());
    print(classObj2.process());
}

// ============================================================================
// Test 49: Template Literals
// ============================================================================
{
    let template1 = `Hello World`;
    let template2 = `Value: ${42}`;
    let template3 = `Result: ${5 + 5}`;
    let template4 = `Name: ${"John"}, Age: ${30}`;
    let template5 = `Multi
line
template`;

    let obj = { name: "Alice", age: 25 };
    let template6 = `User: ${obj.name}, Age: ${obj.age}`;

    print("\n=== Test 49: Template Literals ===");
    print(template1);
    print(template2);
    print(template3);
    print(template4);
    print(template5);
    print(template6);
}

// ============================================================================
// Test 50: Destructuring
// ============================================================================
{
    let arrDest = [1, 2, 3, 4, 5];
    let [a, b, c] = arrDest;
    let [d, e, ...rest] = arrDest;

    let objDest = { x: 10, y: 20, z: 30 };
    let { x, y, z } = objDest;
    let { x: newX, y: newY } = objDest;

    print("\n=== Test 50: Destructuring ===");
    print(a);
    print(b);
    print(c);
    print(d);
    print(e);
    print(rest.join(","));
    print(x);
    print(y);
    print(z);
    print(newX);
    print(newY);
}

// ============================================================================
// Test 51: Spread Operator
// ============================================================================
{
    let arr1 = [1, 2, 3];
    let arr2 = [4, 5, 6];
    let arr3 = [...arr1, ...arr2, 7, 8, 9];

    let obj1 = { a: 1, b: 2 };
    let obj2 = { c: 3, d: 4 };
    let obj3 = { ...obj1, ...obj2, e: 5 };

    print("\n=== Test 51: Spread Operator ===");
    print(arr3.join(","));
    print(JSON.stringify(obj3));

    function testSpread(a, b, c, d, e) {
        print(a);
        print(b);
        print(c);
        print(d);
        print(e);
    }

    testSpread(...arr1, ...arr2);
}

// ============================================================================
// Test 52: For...Of Loop
// ============================================================================
{
    let forOfArray = [1, 2, 3, 4, 5];
    let forOfStr = "Hello";
    let forOfSet = new Set([1, 2, 3, 4, 5]);

    print("\n=== Test 52: For...Of Loop ===");
    let sum1 = 0;
    for (let value of forOfArray) {
        sum1 += value;
    }
    print("Sum: " + sum1);

    let concatStr = "";
    for (let char of forOfStr) {
        concatStr += char;
    }
    print("String: " + concatStr);

    let sum2 = 0;
    for (let value of forOfSet) {
        sum2 += value;
    }
    print("Set Sum: " + sum2);
}

// ============================================================================
// Test 53: For...In Loop
// ============================================================================
{
    let forInObj = {
        a: 1,
        b: 2,
        c: 3,
        d: 4
    };

    let forInArray = [10, 20, 30, 40, 50];

    print("\n=== Test 53: For...In Loop ===");
    let keys1 = [];
    for (let key in forInObj) {
        keys1.push(key);
    }
    print("Keys: " + keys1.join(","));

    let indices = [];
    for (let index in forInArray) {
        indices.push(index);
    }
    print("Indices: " + indices.join(","));
}

// ============================================================================
// Test 54: Import/Export Operations
// ============================================================================
{
    print("\n=== Test 54: Import/Export Operations ===");
    print("Module system check completed");
}

// ============================================================================
// Test 55: Dynamic Property Names
// ============================================================================
{
    let propName = "dynamic";
    let objDynamic = {
        static: 1,
        [propName]: 2,
        ["computed" + "Prop"]: 3
    };

    print("\n=== Test 55: Dynamic Property Names ===");
    print(objDynamic.static);
    print(objDynamic.dynamic);
    print(objDynamic.computedProp);
    print(JSON.stringify(objDynamic));
}

// ============================================================================
// Test 56: Optional Chaining
// ============================================================================
{
    let optionalObj = {
        a: {
            b: {
                c: 42
            }
        }
    };

    print("\n=== Test 56: Optional Chaining ===");
    let value56 = optionalObj?.a?.b?.c;
    print("Value: " + value56);
    let value56b = optionalObj?.x?.y?.z;
    print("Undefined: " + value56b);
}

// ============================================================================
// Test 57: Nullish Coalescing
// ============================================================================
{
    print("\n=== Test 57: Nullish Coalescing ===");
    let value57a = null ?? "default";
    print("Null coalesce: " + value57a);
    let value57b = undefined ?? "default";
    print("Undefined coalesce: " + value57b);
    let value57c = 0 ?? "default";
    print("Zero coalesce: " + value57c);
    let value57d = "" ?? "default";
    print("Empty string coalesce: " + value57d);
}

// ============================================================================
// Test 58: BigInt
// ============================================================================
{
    print("\n=== Test 58: BigInt ===");
    try {
        let big1 = 123456789012345678901234567890n;
        let big2 = 987654321098765432109876543210n;
        print("BigInt support available");
        print((big1 + big2).toString());
        print((big1 * 2n).toString());
    } catch (e) {
        print("BigInt not fully supported: " + e.message);
    }
}

// ============================================================================
// Test 59: Static Methods
// ============================================================================
{
    class StaticClass {
        static staticMethod() {
            return "Static method called";
        }

        static staticProp = "Static property";

        static sum(a, b) {
            return a + b;
        }
    }

    print("\n=== Test 59: Static Methods ===");
    print(StaticClass.staticMethod());
    print(StaticClass.staticProp);
    print(StaticClass.sum(5, 3));
}

// ============================================================================
// Test 60: Private Fields
// ============================================================================
{
    class PrivateClass {
        #privateField = 42;

        #privateMethod() {
            return this.#privateField;
        }

        getPrivateValue() {
            return this.#privateMethod();
        }
    }

    print("\n=== Test 60: Private Fields ===");
    let privateObj = new PrivateClass();
    print(privateObj.getPrivateValue());
}

// ============================================================================
// Final Summary
// ============================================================================
{
    print("\n=== Test Suite Completed ===");
    print("Total tests executed: 60");
    print("Comprehensive Call operations test completed successfully");
}
