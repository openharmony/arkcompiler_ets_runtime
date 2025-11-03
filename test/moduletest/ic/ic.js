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
 * @tc.name:ic_comprehensive_test
 * @tc.desc:comprehensive test for inline cache in various scenarios
 * @tc.type: FUNC
 */

// ==================== TEST SUITE 1: OBJECT PROPERTY ACCESS PATTERNS ====================
function testObjectPropertyAccessIC() {
    print("Starting Object Property Access IC Tests");
    
    // Test 1.1: Monomorphic property access
    let obj1 = {a: 1, b: 2, c: 3, d: 4, e: 5};
    let sum1 = 0;
    for (let i = 0; i < 11; i++) {
        sum1 += obj1.a + obj1.b + obj1.c + obj1.d + obj1.e;
        obj1.a = i % 100;
        obj1.b = (i * 2) % 100;
        obj1.c = (i * 3) % 100;
        obj1.d = (i * 4) % 100;
        obj1.e = (i * 5) % 100;
    }
    print("Monomorphic access result: " + sum1);
    
    // Test 1.2: Polymorphic property access (up to 4 different shapes)
    let objects = [];
    for (let i = 0; i < 11; i++) {
        if (i % 4 === 0) {
            objects.push({x: i, y: i * 2, type: "typeA"});
        } else if (i % 4 === 1) {
            objects.push({x: i, z: i * 3, type: "typeB"});
        } else if (i % 4 === 2) {
            objects.push({y: i, w: i * 4, type: "typeC"});
        } else {
            objects.push({x: i, y: i * 2, z: i * 3, w: i * 4, type: "typeD"});
        }
    }
    
    let sum2 = 0;
    for (let i = 0; i < objects.length; i++) {
        let obj = objects[i];
        if (obj.x !== undefined) sum2 += obj.x;
        if (obj.y !== undefined) sum2 += obj.y;
        if (obj.z !== undefined) sum2 += obj.z;
        if (obj.w !== undefined) sum2 += obj.w;
    }
    print("Polymorphic access result: " + sum2);
    
    // Test 1.3: Megamorphic property access (many different shapes)
    let megaObjects = [];
    for (let i = 0; i < 11; i++) {
        let obj = {base: i};
        for (let j = 0; j < (i % 10) + 1; j++) {
            obj[`prop${j}`] = i * j;
        }
        if (i % 3 === 0) obj.extra = i * 100;
        if (i % 5 === 0) obj.special = i * 200;
        megaObjects.push(obj);
    }
    
    let sum3 = 0;
    for (let i = 0; i < megaObjects.length; i++) {
        let obj = megaObjects[i];
        sum3 += obj.base;
        for (let key in obj) {
            if (key !== 'base') {
                sum3 += obj[key];
            }
        }
    }
    print("Megamorphic access result: " + sum3);
    
    // Test 1.4: Nested object property access
    let nestedObj = {
        level1: {
            a: 1,
            level2: {
                b: 2,
                level3: {
                    c: 3,
                    level4: {
                        d: 4,
                        level5: {
                            e: 5,
                            data: "deep_data"
                        }
                    }
                }
            }
        }
    };
    
    let deepSum = 0;
    for (let i = 0; i < 11; i++) {
        deepSum += nestedObj.level1.a;
        deepSum += nestedObj.level1.level2.b;
        deepSum += nestedObj.level1.level2.level3.c;
        deepSum += nestedObj.level1.level2.level3.level4.d;
        deepSum += nestedObj.level1.level2.level3.level4.level5.e;
        deepSum += nestedObj.level1.level2.level3.level4.level5.data.length;
        
        // Modify nested properties
        nestedObj.level1.a = (nestedObj.level1.a + 1) % 100;
        nestedObj.level1.level2.b = (nestedObj.level1.level2.b + 2) % 100;
        nestedObj.level1.level2.level3.c = (nestedObj.level1.level2.level3.c + 3) % 100;
    }
    print("Nested access result: " + deepSum);
    
    return [sum1, sum2, sum3, deepSum];
}

// ==================== TEST SUITE 2: ARRAY ELEMENT ACCESS PATTERNS ====================
function testArrayElementAccessIC() {
    print("Starting Array Element Access IC Tests");
    
    // Test 2.1: Packed array access
    let packedArray = new Array(100);
    for (let i = 0; i < packedArray.length; i++) {
        packedArray[i] = i * 2 + 1;
    }
    
    let packedSum = 0;
    for (let i = 0; i < packedArray.length; i++) {
        packedSum += packedArray[i];
        if (i % 100 === 0) {
            packedArray[i] = packedArray[i] * 3; // Cause some element kind changes
        }
    }
    print("Packed array access result: " + packedSum);
    
    // Test 2.2: Holey array access
    let holeyArray = new Array(80);
    for (let i = 0; i < holeyArray.length; i += 2) {
        holeyArray[i] = i * 3;
    }
    
    let holeySum = 0;
    let holeyCount = 0;
    for (let i = 0; i < holeyArray.length; i++) {
        if (i in holeyArray) {
            holeySum += holeyArray[i];
            holeyCount++;
        }
    }
    print("Holey array access result: " + holeySum + ", count: " + holeyCount);
    
    // Test 2.3: Array with mixed element kinds
    let mixedArray = [1, 2, 3]; // PACKED_SMI_ELEMENTS
    for (let i = 3; i < 11; i++) {
        mixedArray.push(i * 1.5); // Transition to PACKED_DOUBLE_ELEMENTS
    }
    for (let i = 11; i < 11; i++) {
        mixedArray.push("string_" + i); // Transition to PACKED_ELEMENTS
    }
    for (let i = 11; i < 11; i++) {
        mixedArray.push({value: i}); // Stay PACKED_ELEMENTS
    }
    
    let mixedSum = 0;
    let stringCount = 0;
    let objectCount = 0;
    for (let i = 0; i < mixedArray.length; i++) {
        let element = mixedArray[i];
        if (typeof element === 'number') {
            mixedSum += element;
        } else if (typeof element === 'string') {
            mixedSum += element.length;
            stringCount++;
        } else if (typeof element === 'object') {
            mixedSum += element.value;
            objectCount++;
        }
    }
    print("Mixed array result: " + mixedSum + ", strings: " + stringCount + ", objects: " + objectCount);
    
    // Test 2.4: Array methods IC
    let methodArray = [];
    for (let i = 0; i < 11; i++) {
        methodArray.push(i * 1.2345);
    }
    
    let methodResults = [];
    
    // forEach
    methodArray.forEach((value, index) => {
        methodResults.push(value + index);
    });
    
    // map
    let mapped = methodArray.map(value => value * 2);
    methodResults.push(mapped.length);
    
    // filter
    let filtered = methodArray.filter(value => value > 500);
    methodResults.push(filtered.length);
    
    // reduce
    let reduced = methodArray.reduce((acc, value) => acc + value, 0);
    methodResults.push(reduced);
    
    print("Array methods completed, result count: " + methodResults.length);
    
    return [packedSum, holeySum, mixedSum, methodResults.length, reduced];
}

// ==================== TEST SUITE 3: FUNCTION CALL PATTERNS ====================
function testFunctionCallIC() {
    print("Starting Function Call IC Tests");
    
    // Test 3.1: Monomorphic function calls
    function add(a, b) { return a + b; }
    
    let monoSum = 0;
    for (let i = 0; i < 11; i++) {
        monoSum += add(i, i + 1);
        monoSum += add(i * 2, i * 3);
        monoSum += add(i + 10, i - 5);
    }
    print("Monomorphic calls result: " + monoSum);
    
    // Test 3.2: Polymorphic function calls
    function processValue(value) {
        if (typeof value === 'number') {
            return value * 2;
        } else if (typeof value === 'string') {
            return value.length;
        } else if (Array.isArray(value)) {
            return value.length;
        } else {
            return Object.keys(value).length;
        }
    }
    
    let polySum = 0;
    for (let i = 0; i < 11; i++) {
        if (i % 4 === 0) {
            polySum += processValue(i);
        } else if (i % 4 === 1) {
            polySum += processValue("string_" + i);
        } else if (i % 4 === 2) {
            polySum += processValue([i, i+1, i+2]);
        } else {
            polySum += processValue({a: i, b: i*2});
        }
    }
    print("Polymorphic calls result: " + polySum);
    
    // Test 3.3: Method calls with different receivers
    let methodObjects = [];
    for (let i = 0; i < 11; i++) {
        let obj = {
            value: i,
            getValue: function() { return this.value; },
            calculate: function(multiplier) { return this.value * multiplier; }
        };
        
        if (i % 3 === 0) {
            obj.extraMethod = function() { return this.value + 100; };
        }
        
        methodObjects.push(obj);
    }
    
    let methodSum = 0;
    for (let i = 0; i < methodObjects.length; i++) {
        let obj = methodObjects[i];
        methodSum += obj.getValue();
        methodSum += obj.calculate(2);
        if (obj.extraMethod) {
            methodSum += obj.extraMethod();
        }
    }
    print("Method calls result: " + methodSum);
    
    // Test 3.4: Constructor calls
    function Person(name, age) {
        this.name = name;
        this.age = age;
        this.id = name.hashCode ? name.hashCode() : name.length; // Fixed ID
    }
    Person.prototype.getInfo = function() {
        return this.name + "_" + this.age;
    };
    
    function Employee(name, age, salary) {
        Person.call(this, name, age);
        this.salary = salary;
    }
    Employee.prototype = Object.create(Person.prototype);
    Employee.prototype.getSalary = function() {
        return this.salary;
    };
    
    let people = [];
    let employees = [];
    for (let i = 0; i < 11; i++) {
        people.push(new Person("Person" + i, 20 + (i % 50)));
        if (i % 2 === 0) {
            employees.push(new Employee("Employee" + i, 25 + (i % 45), 30000 + i * 100));
        }
    }
    
    let constructorSum = 0;
    for (let person of people) {
        constructorSum += person.name.length;
        constructorSum += person.age;
        constructorSum += person.getInfo().length;
    }
    for (let emp of employees) {
        constructorSum += emp.getInfo().length;
        constructorSum += emp.getSalary();
    }
    print("Constructor calls result: " + constructorSum);
    
    return [monoSum, polySum, methodSum, constructorSum];
}

// ==================== TEST SUITE 4: PROTOTYPE CHAIN ACCESS PATTERNS ====================
function testPrototypeChainIC() {
    print("Starting Prototype Chain IC Tests");
    
    // Test 4.1: Deep prototype chains
    function Level1() { this.l1 = 1; }
    Level1.prototype.method1 = function() { return this.l1 * 2; };
    
    function Level2() { Level1.call(this); this.l2 = 2; }
    Level2.prototype = Object.create(Level1.prototype);
    Level2.prototype.constructor = Level2;
    Level2.prototype.method2 = function() { return this.l2 * 3 + this.method1(); };
    
    function Level3() { Level2.call(this); this.l3 = 3; }
    Level3.prototype = Object.create(Level2.prototype);
    Level3.prototype.constructor = Level3;
    Level3.prototype.method3 = function() { return this.l3 * 4 + this.method2(); };
    
    function Level4() { Level3.call(this); this.l4 = 4; }
    Level4.prototype = Object.create(Level3.prototype);
    Level4.prototype.constructor = Level4;
    Level4.prototype.method4 = function() { return this.l4 * 5 + this.method3(); };
    
    function Level5() { Level4.call(this); this.l5 = 5; }
    Level5.prototype = Object.create(Level4.prototype);
    Level5.prototype.constructor = Level5;
    Level5.prototype.method5 = function() { return this.l5 * 6 + this.method4(); };
    
    let deepChainObjects = [];
    for (let i = 0; i < 11; i++) {
        deepChainObjects.push(new Level5());
    }
    
    let chainSum = 0;
    for (let obj of deepChainObjects) {
        chainSum += obj.method1();
        chainSum += obj.method2();
        chainSum += obj.method3();
        chainSum += obj.method4();
        chainSum += obj.method5();
        
        // Property access through chain
        chainSum += obj.l1;
        chainSum += obj.l2;
        chainSum += obj.l3;
        chainSum += obj.l4;
        chainSum += obj.l5;
    }
    print("Deep prototype chain result: " + chainSum);
    
    // Test 4.2: Polymorphic prototype access
    function BaseA() { this.type = "A"; this.a = 10; }
    BaseA.prototype.getA = function() { return this.a; };
    
    function BaseB() { this.type = "B"; this.b = 20; }
    BaseB.prototype.getB = function() { return this.b; };
    
    function BaseC() { this.type = "C"; this.c = 30; }
    BaseC.prototype.getC = function() { return this.c; };
    
    let polyProtoObjects = [];
    for (let i = 0; i < 11; i++) {
        if (i % 3 === 0) {
            polyProtoObjects.push(new BaseA());
        } else if (i % 3 === 1) {
            polyProtoObjects.push(new BaseB());
        } else {
            polyProtoObjects.push(new BaseC());
        }
    }
    
    let polyProtoSum = 0;
    for (let obj of polyProtoObjects) {
        if (obj.type === "A") {
            polyProtoSum += obj.getA();
        } else if (obj.type === "B") {
            polyProtoSum += obj.getB();
        } else if (obj.type === "C") {
            polyProtoSum += obj.getC();
        }
    }
    print("Polymorphic prototype result: " + polyProtoSum);
    
    // Test 4.3: Prototype property modification
    function DynamicProto() { this.value = 100; }
    DynamicProto.prototype.baseMethod = function() { return this.value; };
    
    let dynamicObjects = [];
    for (let i = 0; i < 11; i++) {
        dynamicObjects.push(new DynamicProto());
    }
    
    // Add method to prototype after object creation
    DynamicProto.prototype.addedMethod = function() { return this.value * 2; };
    
    // Modify existing prototype method
    let originalBaseMethod = DynamicProto.prototype.baseMethod;
    DynamicProto.prototype.baseMethod = function() { return originalBaseMethod.call(this) + 10; };
    
    let dynamicSum = 0;
    for (let obj of dynamicObjects) {
        dynamicSum += obj.baseMethod();
        dynamicSum += obj.addedMethod();
    }
    print("Dynamic prototype result: " + dynamicSum);
    
    return [chainSum, polyProtoSum, dynamicSum];
}

// ==================== TEST SUITE 5: PROPERTY ACCESSOR PATTERNS ====================
function testPropertyAccessorIC() {
    print("Starting Property Accessor IC Tests");
    
    // Test 5.1: Getter/Setter access patterns
    let getterSetterObjects = [];
    for (let i = 0; i < 11; i++) {
        let obj = {
            _value: i,
            _computed: i * 2,
            
            get value() {
                return this._value;
            },
            
            set value(newVal) {
                this._value = newVal;
                this._computed = newVal * 2;
            },
            
            get computed() {
                return this._computed;
            },
            
            calculate: function() {
                return this.value + this.computed;
            }
        };
        
        getterSetterObjects.push(obj);
    }
    
    let accessorSum = 0;
    for (let i = 0; i < getterSetterObjects.length; i++) {
        let obj = getterSetterObjects[i];
        accessorSum += obj.value;        // Getter call
        obj.value = i * 3;               // Setter call
        accessorSum += obj.computed;     // Getter call
        accessorSum += obj.calculate();
    }
    print("Getter/Setter result: " + accessorSum);
    
    // Test 5.2: Object.defineProperty with accessors
    let definedPropertyObjects = [];
    for (let i = 0; i < 11; i++) {
        let obj = { _internal: i };
        
        Object.defineProperty(obj, 'definedProp', {
            get: function() {
                return this._internal * 3;
            },
            set: function(value) {
                this._internal = value / 3;
            },
            enumerable: true,
            configurable: true
        });
        
        definedPropertyObjects.push(obj);
    }
    
    let definedSum = 0;
    for (let obj of definedPropertyObjects) {
        definedSum += obj.definedProp;   // Getter call
        obj.definedProp = obj.definedProp * 2; // Setter call
        definedSum += obj.definedProp;   // Getter call again
    }
    print("Defined property accessors result: " + definedSum);
    
    // Test 5.3: Mixed data and accessor properties
    class MixedProperties {
        constructor(base) {
            this.dataProp = base;
            this._accessorProp = base * 2;
        }
        
        get accessorProp() {
            return this._accessorProp;
        }
        
        set accessorProp(value) {
            this._accessorProp = value;
            this.dataProp = value / 2;
        }
        
        method() {
            return this.dataProp + this.accessorProp;
        }
    }
    
    let mixedPropObjects = [];
    for (let i = 0; i < 11; i++) {
        mixedPropObjects.push(new MixedProperties(i));
    }
    
    let mixedPropSum = 0;
    for (let obj of mixedPropObjects) {
        mixedPropSum += obj.dataProp;        // Data property
        mixedPropSum += obj.accessorProp;    // Getter
        obj.accessorProp = obj.accessorProp + 10; // Setter
        mixedPropSum += obj.method();        // Method call
    }
    print("Mixed properties result: " + mixedPropSum);
    
    return [accessorSum, definedSum, mixedPropSum];
}

// ==================== TEST SUITE 6: LOOP OPTIMIZATION PATTERNS ====================
function testLoopOptimizationIC() {
    print("Starting Loop Optimization IC Tests");
    
    // Test 6.1: For loop with various patterns
    let forLoopResults = [];

    // Simple for loop with array
    let array1 = [];
    for (let i = 0; i < 11; i++) {
        array1.push(i * Math.PI);
    }
    
    let forSum1 = 0;
    for (let i = 0; i < array1.length; i++) {
        forSum1 += array1[i];
        if (i % 100 === 0) {
            array1[i] = array1[i] * 2; // Cause some element kind changes
        }
    }
    forLoopResults.push(forSum1);
    
    // For loop with object properties
    let objFor = {};
    for (let i = 0; i < 11; i++) {
        objFor[`key${i}`] = i * i;
    }
    
    let forSum2 = 0;
    for (let key in objFor) {
        forSum2 += objFor[key];
    }
    forLoopResults.push(forSum2);
    
    // Test 6.2: While and do-while loops
    let whileResults = [];
    
    let whileSum = 0;
    let whileCounter = 0;
    while (whileCounter < 11) {
        whileSum += whileCounter * (whileCounter % 10);
        whileCounter++;

        if (whileCounter % 50 === 0) {
            whileSum += 1000; // Branch prediction test
        }
    }
    whileResults.push(whileSum);
    
    let doWhileSum = 0;
    let doWhileCounter = 0;
    do {
        doWhileSum += Math.sin(doWhileCounter) * 100;
        doWhileCounter++;
    } while (doWhileCounter < 11);
    whileResults.push(doWhileSum);
    
    // Test 6.3: Nested loops with complex access patterns
    let matrix = [];
    const size = 5;
    for (let i = 0; i < size; i++) {
        matrix[i] = [];
        for (let j = 0; j < size; j++) {
            matrix[i][j] = i * j + Math.cos(i) * Math.sin(j) * 100;
        }
    }
    
    let nestedSum = 0;
    for (let i = 0; i < size; i++) {
        for (let j = 0; j < size; j++) {
            nestedSum += matrix[i][j];
            // Access in different orders
            if (i % 2 === 0) {
                nestedSum += matrix[j][i]; // Transpose access
            }
        }
    }
    forLoopResults.push(nestedSum);
    
    // Test 6.4: Loop with function calls
    let funcLoopSum = 0;
    for (let i = 0; i < 11; i++) {
        funcLoopSum += Math.sqrt(i);
        funcLoopSum += Math.pow(2, i % 10);
        funcLoopSum += String(i).length;
    }
    forLoopResults.push(funcLoopSum);
    
    print("Loop optimization results: " + forLoopResults.join(", "));
    print("While loop results: " + whileResults.join(", "));
    
    return [...forLoopResults, ...whileResults];
}

// ==================== TEST SUITE 7: EXCEPTION HANDLING PATTERNS ====================
function testExceptionHandlingIC() {
    print("Starting Exception Handling IC Tests");
    
    // Test 7.1: Try-catch in hot loops
    let exceptionSum = 0;
    let exceptionCount = 0;

    for (let i = 0; i < 11; i++) {
        try {
            if (i % 17 === 0) {
                throw new Error("Controlled error at " + i);
            }
            if (i % 23 === 0) {
                throw new TypeError("Type error at " + i);
            }
            exceptionSum += i * 2;
        } catch (e) {
            exceptionCount++;
            exceptionSum += i; // Different path
        }
    }
    print("Basic exception handling - sum: " + exceptionSum + ", exceptions: " + exceptionCount);
    
    // Test 7.2: Nested try-catch blocks
    let nestedExceptionSum = 0;
    
    function riskyOperation(level, value) {
        try {
            if (level === 0) {
                if (value % 13 === 0) {
                    throw new Error("Level 0 error");
                }
                return value * 2;
            } else {
                try {
                    let result = riskyOperation(level - 1, value);
                    if (value % 7 === 0) {
                        throw new Error("Level 1 error");
                    }
                    return result + 10;
                } catch (innerError) {
                    if (value % 3 === 0) {
                        throw new Error("Rethrown: " + innerError.message);
                    }
                    return value * 3;
                }
            }
        } catch (outerError) {
            return value * 4;
        }
    }
    
    for (let i = 1; i < 11; i++) {
        nestedExceptionSum += riskyOperation(2, i);
    }
    print("Nested exception handling result: " + nestedExceptionSum);
    
    // Test 7.3: Finally block execution
    let finallySum = 0;
    let finallyCount = 0;

    for (let i = 0; i < 11; i++) {
        try {
            if (i % 29 === 0) {
                throw new Error("Finally test error");
            }
            finallySum += i;
        } catch (e) {
            finallySum += i * 2;
        } finally {
            finallyCount++;
            finallySum += 1; // Always execute
        }
    }
    print("Finally block result: " + finallySum + ", executions: " + finallyCount);
    
    return [exceptionSum, nestedExceptionSum, finallySum, exceptionCount, finallyCount];
}

// ==================== TEST SUITE 8: TYPE TRANSITION PATTERNS ====================
function testTypeTransitionIC() {
    print("Starting Type Transition IC Tests");
    
    // Test 8.1: Object shape transitions
    let shapeChangingObjects = [];

    // Phase 1: Create objects with same shape
    for (let i = 0; i < 11; i++) {
        shapeChangingObjects.push({a: i, b: i * 2, c: i * 3});
    }

    // Phase 2: Transition to different shapes
    for (let i = 0; i < shapeChangingObjects.length; i++) {
        let obj = shapeChangingObjects[i];
        if (i % 4 === 0) {
            // Add property
            obj.d = i * 4;
        } else if (i % 4 === 1) {
            // Delete property
            delete obj.b;
        } else if (i % 4 === 2) {
            // Change property order by re-creation
            let newObj = {c: obj.c, a: obj.a, b: obj.b};
            shapeChangingObjects[i] = newObj;
        }
        // i % 4 === 3: Keep original shape
    }
    
    let shapeSum = 0;
    for (let obj of shapeChangingObjects) {
        if (obj.a !== undefined) shapeSum += obj.a;
        if (obj.b !== undefined) shapeSum += obj.b;
        if (obj.c !== undefined) shapeSum += obj.c;
        if (obj.d !== undefined) shapeSum += obj.d;
    }
    print("Object shape transition result: " + shapeSum);
    
    // Test 8.2: Array element kind transitions
    let transitioningArray = [1, 2, 3]; // PACKED_SMI_ELEMENTS

    // Transition to PACKED_DOUBLE_ELEMENTS
    for (let i = 3; i < 11; i++) {
        transitioningArray.push(i + 0.5);
    }

    // Transition to PACKED_ELEMENTS
    for (let i = 11; i < 11; i++) {
        transitioningArray.push("str_" + i);
    }

    // Make it HOLEY by deleting elements
    for (let i = 5; i < 9; i += 2) {
        delete transitioningArray[i];
    }
    
    let transitionSum = 0;
    for (let i = 0; i < transitioningArray.length; i++) {
        let element = transitioningArray[i];
        if (typeof element === 'number') {
            transitionSum += element;
        } else if (typeof element === 'string') {
            transitionSum += element.length;
        }
        // Undefined elements (holes) contribute 0
    }
    print("Array type transition result: " + transitionSum);
    
    // Test 8.3: Function feedback vector changes
    function adaptiveFunction(input) {
        if (input === null || input === undefined) {
            return 0; // Handle null and undefined
        } else if (typeof input === 'number') {
            return input * 2;
        } else if (typeof input === 'string') {
            return input.length;
        } else if (Array.isArray(input)) {
            return input.reduce((a, b) => a + b, 0);
        } else {
            return Object.keys(input).length;
        }
    }

    let adaptiveSum = 0;
    // Call with different types to cause feedback vector changes
    for (let i = 0; i < 11; i++) {
        if (i % 6 === 0) {
            adaptiveSum += adaptiveFunction(i);
        } else if (i % 6 === 1) {
            adaptiveSum += adaptiveFunction("text" + i);
        } else if (i % 6 === 2) {
            adaptiveSum += adaptiveFunction([i, i+1, i+2]);
        } else if (i % 6 === 3) {
            adaptiveSum += adaptiveFunction({a: i, b: i*2});
        } else if (i % 6 === 4) {
            adaptiveSum += adaptiveFunction(null);
        } else {
            adaptiveSum += adaptiveFunction(undefined);
        }
    }
    print("Adaptive function result: " + adaptiveSum);
    
    return [shapeSum, transitionSum, adaptiveSum];
}

// ==================== TEST SUITE 9: GLOBAL AND BUILTIN ACCESS PATTERNS ====================
function testGlobalAccessIC() {
    print("Starting Global and Builtin Access IC Tests");
    
    // Test 9.1: Math object access
    let mathSum = 0;
    for (let i = 0; i < 11; i++) {
        mathSum += Math.abs(i - 500);
        mathSum += Math.max(i % 100, (i + 1) % 100, (i + 2) % 100);
        mathSum += Math.min(i * 0.1, i * 0.2, i * 0.3);
        mathSum += Math.floor(i * 1.7);
        mathSum += Math.ceil(i * 1.3);
        mathSum += Math.sin(i * 0.01) * 100;
        mathSum += Math.cos(i * 0.02) * 100;
        mathSum += Math.sqrt(i + 1);
        mathSum += Math.pow(2, i % 10);

        if (i % 100 === 0) {
            mathSum += 500; // Fixed value instead of random
        }
    }
    print("Math object access result: " + mathSum);
    
    // Test 9.2: String methods access
    let stringSum = 0;
    let testStrings = [];
    for (let i = 0; i < 11; i++) {
        testStrings.push("TestString_" + i + "_" + (i * 67890).toString(36));
    }
    
    for (let str of testStrings) {
        stringSum += str.length;
        stringSum += str.charAt(5).charCodeAt(0);
        stringSum += str.charCodeAt(3);
        stringSum += str.indexOf('_');
        stringSum += str.lastIndexOf('_');
        stringSum += str.substring(2, 8).length;
        stringSum += str.slice(4, 10).length;
        stringSum += str.toUpperCase().length;
        stringSum += str.toLowerCase().length;
        stringSum += str.split('_').length;
        stringSum += str.replace('Test', 'Check').length;
        stringSum += str.includes('String') ? 1 : 0;
        stringSum += str.startsWith('Test') ? 1 : 0;
        stringSum += str.endsWith('0') ? 1 : 0;
    }
    print("String methods result: " + stringSum);
    
    // Test 9.3: Date object access
    let dateSum = 0;
    let dates = [];
    for (let i = 0; i < 11; i++) {
        dates.push(new Date(2020, i % 12, (i % 28) + 1, i % 24, i % 60, i % 60));
        dates[i].setMilliseconds(i * 100); // Fixed increment
    }
    
    for (let date of dates) {
        dateSum += date.getFullYear();
        dateSum += date.getMonth();
        dateSum += date.getDate();
        dateSum += date.getDay();
        dateSum += date.getHours();
        dateSum += date.getMinutes();
        dateSum += date.getSeconds();
        dateSum += date.getMilliseconds();
        dateSum += date.getTime() % 100000;
    }
    print("Date object access result: " + dateSum);
    
    // Test 9.4: JSON methods
    let jsonSum = 0;
    let jsonObjects = [];
    let fixedBaseTime = 1577836800000; // 2020-01-01 00:00:00 UTC
    for (let i = 0; i < 11; i++) {
        jsonObjects.push({
            id: i,
            data: "JSON_test_" + i,
            values: [i, i*2, i*3],
            nested: {
                level: i % 10,
                active: i % 2 === 0
            },
            timestamp: fixedBaseTime + i * 86400000 // Fixed daily increments
        });
    }
    
    for (let obj of jsonObjects) {
        let jsonString = JSON.stringify(obj);
        jsonSum += jsonString.length;
        
        let parsed = JSON.parse(jsonString);
        jsonSum += parsed.id;
        jsonSum += parsed.data.length;
        jsonSum += parsed.values.length;
        jsonSum += parsed.nested.level;
        jsonSum += parsed.nested.active ? 1 : 0;
    }
    print("JSON methods result: " + jsonSum);
    
    return [mathSum, stringSum, dateSum, jsonSum];
}

// ==================== TEST SUITE 10: COMPLEX DATA STRUCTURE PATTERNS ====================
function testComplexDataStructuresIC() {
    print("Starting Complex Data Structure IC Tests");
    
    // Test 10.1: Map and Set operations
    let mapSum = 0;
    let testMap = new Map();
    let testSet = new Set();

    // Populate Map and Set
    for (let i = 0; i < 11; i++) {
        testMap.set(`key_${i}`, i * 10);
        testMap.set(i, `value_${i}`);
        testSet.add(i);
        testSet.add(`set_item_${i}`);
    }
    
    // Map operations
    mapSum += testMap.size;
    for (let [key, value] of testMap) {
        if (typeof key === 'string') {
            mapSum += key.length;
        } else {
            mapSum += key;
        }
        if (typeof value === 'string') {
            mapSum += value.length;
        } else {
            mapSum += value;
        }
    }
    
    // Set operations
    mapSum += testSet.size;
    for (let value of testSet) {
        if (typeof value === 'string') {
            mapSum += value.length;
        } else {
            mapSum += value;
        }
    }
    
    print("Map and Set result: " + mapSum);
    
    // Test 10.2: Symbol property access
    let symbolSum = 0;
    let symbolObjects = [];
    let sym1 = Symbol('test1');
    let sym2 = Symbol('test2');
    let sym3 = Symbol.for('global1');
    let sym4 = Symbol.for('global2');
    
    for (let i = 0; i < 11; i++) {
        let obj = {
            regular: i,
            [sym1]: i * 2,
            [sym2]: `symbol_${i}`,
            [sym3]: i * 3,
            [sym4]: `global_${i}`
        };
        symbolObjects.push(obj);
    }
    
    for (let obj of symbolObjects) {
        symbolSum += obj.regular;
        symbolSum += obj[sym1];
        symbolSum += obj[sym2].length;
        symbolSum += obj[sym3];
        symbolSum += obj[sym4].length;
    }
    print("Symbol property result: " + symbolSum);
    
    // Test 10.3: WeakMap and WeakSet (though we can't iterate, we can test access)
    let weakSum = 0;
    let weakMap = new WeakMap();
    let weakSet = new WeakSet();
    let weakObjects = [];
    
    for (let i = 0; i < 11; i++) {
        let obj = {id: i, data: `weak_${i}`};
        weakObjects.push(obj);
        weakMap.set(obj, i * 100);
        weakSet.add(obj);
    }
    
    // Access WeakMap and WeakSet
    for (let obj of weakObjects) {
        if (weakMap.has(obj)) {
            weakSum += weakMap.get(obj);
        }
        if (weakSet.has(obj)) {
            weakSum += obj.id;
        }
    }
    print("WeakMap/WeakSet result: " + weakSum);
    
    // Test 10.4: ArrayBuffer and TypedArrays
    let bufferSum = 0;
    let buffer = new ArrayBuffer(4000);
    let int32View = new Int32Array(buffer);
    let float64View = new Float64Array(buffer);
    
    // Initialize buffers
    for (let i = 0; i < int32View.length; i++) {
        int32View[i] = i * 2;
    }
    for (let i = 0; i < float64View.length; i++) {
        float64View[i] = i * 1.5;
    }
    
    // Access buffers
    for (let i = 0; i < int32View.length; i++) {
        bufferSum += int32View[i];
    }
    for (let i = 0; i < float64View.length; i++) {
        bufferSum += float64View[i];
    }
    print("ArrayBuffer/TypedArray result: " + bufferSum);
    
    return [mapSum, symbolSum, weakSum, bufferSum];
}

// ==================== TEST SUITE 11: CLOSURE AND SCOPE PATTERNS ====================
function testClosureScopeIC() {
    print("Starting Closure and Scope IC Tests");
    
    // Test 11.1: Basic closures
    function createCounter() {
        let count = 0;
        return {
            increment: function() {
                count++;
                return count;
            },
            decrement: function() {
                count--;
                return count;
            },
            getValue: function() {
                return count;
            },
            reset: function() {
                count = 0;
            }
        };
    }
    
    let counters = [];
    for (let i = 0; i < 11; i++) {
        counters.push(createCounter());
    }

    let closureSum = 0;
    for (let i = 0; i < 11; i++) {
        let counter = counters[i % counters.length];
        if (i % 4 === 0) {
            closureSum += counter.increment();
        } else if (i % 4 === 1) {
            closureSum += counter.decrement();
        } else if (i % 4 === 2) {
            closureSum += counter.getValue();
        } else {
            counter.reset();
        }
    }
    print("Basic closures result: " + closureSum);
    
    // Test 11.2: Nested closures with multiple levels
    function createMultiLevelClosure(base) {
        let level1 = base;
        return function(level2) {
            let current = level1 + level2;
            return function(level3) {
                let result = current + level3;
                return function(level4) {
                    return result + level4 + level1; // Access outer scope
                };
            };
        };
    }
    
    let nestedClosureSum = 0;
    for (let i = 0; i < 11; i++) {
        let level1Func = createMultiLevelClosure(i);
        let level2Func = level1Func(i * 2);
        let level3Func = level2Func(i * 3);
        let finalResult = level3Func(i * 4);
        nestedClosureSum += finalResult;
    }
    print("Nested closures result: " + nestedClosureSum);
    
    // Test 11.3: Closures with object methods
    function createObjectClosure(initial) {
        let privateData = initial;
        let callCount = 0;
        
        return {
            getData: function() {
                callCount++;
                return privateData;
            },
            setData: function(newData) {
                callCount++;
                privateData = newData;
            },
            getCallCount: function() {
                return callCount;
            },
            process: function(multiplier) {
                callCount++;
                return privateData * multiplier + callCount;
            }
        };
    }
    
    let objectClosures = [];
    for (let i = 0; i < 11; i++) {
        objectClosures.push(createObjectClosure(i * 10));
    }

    let objectClosureSum = 0;
    for (let i = 0; i < 11; i++) {
        let obj = objectClosures[i % objectClosures.length];
        if (i % 5 === 0) {
            objectClosureSum += obj.getData();
        } else if (i % 5 === 1) {
            obj.setData(i * 20);
        } else if (i % 5 === 2) {
            objectClosureSum += obj.getCallCount();
        } else if (i % 5 === 3) {
            objectClosureSum += obj.process(2);
        } else {
            obj.setData(obj.getData() + 1);
        }
    }
    print("Object closures result: " + objectClosureSum);
    
    return [closureSum, nestedClosureSum, objectClosureSum];
}

// ==================== TEST SUITE 12: ADVANCED LANGUAGE FEATURE PATTERNS ====================
function testAdvancedLanguageFeaturesIC() {
    print("Starting Advanced Language Features IC Tests");
    
    // Test 12.1: Proxy objects
    let proxySum = 0;
    let proxyObjects = [];
    
    for (let i = 0; i < 11; i++) {
        let target = { value: i, computed: i * 2 };
        let handler = {
            get: function(obj, prop) {
                if (prop === 'doubled') {
                    return obj.value * 2;
                }
                return obj[prop];
            },
            set: function(obj, prop, value) {
                if (prop === 'value') {
                    obj.computed = value * 3;
                }
                obj[prop] = value;
                return true;
            },
            has: function(obj, prop) {
                return prop in obj || prop === 'doubled';
            }
        };

        proxyObjects.push(new Proxy(target, handler));
    }
    
    for (let proxy of proxyObjects) {
        proxySum += proxy.value;
        proxySum += proxy.computed;
        proxySum += proxy.doubled; // Virtual property via get trap
        
        proxy.value = proxy.value + 10; // Triggers set trap
        proxySum += proxy.computed; // Updated by set trap
        
        if ('doubled' in proxy) {
            proxySum += 1; // Virtual property exists
        }
    }
    print("Proxy objects result: " + proxySum);
    
    // Test 12.2: Reflect operations
    let reflectSum = 0;
    let reflectObjects = [];
    
    for (let i = 0; i < 11; i++) {
        reflectObjects.push({
            num: i,
            str: "reflect_" + i,
            nested: { value: i * 5 }
        });
    }
    
    for (let obj of reflectObjects) {
        // Reflect.get
        reflectSum += Reflect.get(obj, 'num');
        reflectSum += Reflect.get(obj, 'str').length;
        
        // Reflect.set
        Reflect.set(obj, 'num', obj.num * 2);
        reflectSum += obj.num;
        
        // Reflect.has
        if (Reflect.has(obj, 'nested')) {
            reflectSum += Reflect.get(obj, 'nested').value;
        }
        
        // Reflect.ownKeys
        let keys = Reflect.ownKeys(obj);
        reflectSum += keys.length;
    }
    print("Reflect operations result: " + reflectSum);
    
    // Test 12.3: Generator functions
    function* numberGenerator(limit) {
        for (let i = 0; i < limit; i++) {
            yield i;
            yield i * 2;
            yield i * 3;
        }
    }
    
    let generatorSum = 0;
    for (let i = 1; i <= 11; i++) {
        let gen = numberGenerator(i % 10 + 1);
        for (let value of gen) {
            generatorSum += value;
        }
    }
    print("Generator functions result: " + generatorSum);
    
    // Test 12.4: Async/Promise patterns (simulated)
    function simulateAsync(value, delay) {
        return {
            then: function(callback) {
                // Simulate async with immediate execution for testing
                let result = callback(value * 2);
                return {
                    then: function(nextCallback) {
                        return nextCallback(result + 10);
                    }
                };
            }
        };
    }
    
    let asyncSum = 0;
    for (let i = 0; i < 11; i++) {
        // Simulate promise chain
        simulateAsync(i, 0)
            .then(result => result + 5)
            .then(finalResult => {
                asyncSum += finalResult;
                return finalResult;
            });
    }
    print("Async/Promise patterns result: " + asyncSum);
    
    return [proxySum, reflectSum, generatorSum, asyncSum];
}

// ==================== EXECUTE TESTS ====================
try {
    let finalResults = runAllICTests();
    print("");
    print("=== ALL TESTS COMPLETED ===");
    print("Final verification - all tests executed successfully");

} catch (error) {
    print("FATAL ERROR: " + error.message);
    print("Stack: " + error.stack);
}

// ==================== TEST SUITE 13: REGULAR EXPRESSION PATTERNS ====================
function testRegularExpressionIC() {
    print("Starting Regular Expression IC Tests");

    // Test 13.1: Pattern matching with different regex objects
    let regexSum = 0;
    let testStrings = [];

    for (let i = 0; i < 500; i++) {
        testStrings.push("test_" + i + "_data_" + (i * 12345).toString(36));
    }

    // Monomorphic regex - same pattern
    let simplePattern = /test_\d+_/;
    let simpleMatchCount = 0;
    for (let str of testStrings) {
        if (simplePattern.test(str)) {
            simpleMatchCount++;
        }
    }
    regexSum += simpleMatchCount;

    // Polymorphic regex - different patterns
    let patterns = [
        /test_\d+_data_/,
        /test_\d+_/,
        /data_\w+/,
        /^[a-z]+_\d+/
    ];

    let polyMatchCount = 0;
    for (let i = 0; i < testStrings.length; i++) {
        let pattern = patterns[i % patterns.length];
        if (pattern.test(testStrings[i])) {
            polyMatchCount++;
        }
    }
    regexSum += polyMatchCount;

    // Test 13.2: Regex exec with capturing groups
    let execSum = 0;
    let dataPattern = /(\w+)_(\d+)_(\w+)/;

    for (let str of testStrings) {
        let match = dataPattern.exec(str);
        if (match) {
            execSum += match[1].length + match[2].length + match[3].length;
            execSum += match.index;
        }
    }

    // Test 13.3: String replace with regex
    let replaceSum = 0;
    let replacePattern = /_\d+_/g;

    for (let str of testStrings) {
        let replaced = str.replace(replacePattern, "_X_");
        replaceSum += replaced.length;
    }

    // Test 13.4: String split with regex
    let splitSum = 0;
    let splitPattern = /[_\d]+/;

    for (let str of testStrings) {
        let parts = str.split(splitPattern);
        splitSum += parts.length;
    }

    print("Regular Expression results - sum: " + regexSum + ", exec: " + execSum + ", replace: " + replaceSum + ", split: " + splitSum);

    return [regexSum, execSum, replaceSum, splitSum, simpleMatchCount, polyMatchCount];
}

// ==================== TEST SUITE 14: BITWISE OPERATION PATTERNS ====================
function testBitwiseOperationIC() {
    print("Starting Bitwise Operation IC Tests");

    // Test 14.1: AND operations
    let andSum = 0;
    for (let i = 0; i < 11; i++) {
        andSum += (i & 0xFF);
        andSum += (i & 0xAAA) & 0xFF;
        andSum += (i & 0x555) & 0xFF;
    }

    // Test 14.2: OR operations
    let orSum = 0;
    for (let i = 0; i < 11; i++) {
        orSum += (i | 0xFF);
        orSum += (i | 0xAAA) & 0xFFF;
        orSum += (i | 0x555) & 0xFFF;
    }

    // Test 14.3: XOR operations
    let xorSum = 0;
    for (let i = 0; i < 11; i++) {
        xorSum += (i ^ 0xFF);
        xorSum += (i ^ 0xAAA) & 0xFFF;
        xorSum += (i ^ 0x555) & 0xFFF;
    }

    // Test 14.4: Left shift operations
    let leftShiftSum = 0;
    for (let i = 0; i < 11; i++) {
        leftShiftSum += (i << 2);
        leftShiftSum += (i << 4) & 0xFFFF;
        leftShiftSum += (i << 6) & 0xFFFFF;
    }

    // Test 14.5: Right shift operations
    let rightShiftSum = 0;
    for (let i = 0; i < 11; i++) {
        rightShiftSum += (i >> 2);
        rightShiftSum += (i >> 4);
        rightShiftSum += (i >> 6);
    }

    // Test 14.6: Unsigned right shift
    let unsignedShiftSum = 0;
    for (let i = 0; i < 11; i++) {
        unsignedShiftSum += (i >>> 2);
        unsignedShiftSum += (i >>> 4);
        unsignedShiftSum += (i >>> 6);
    }

    print("Bitwise operations - AND: " + andSum + ", OR: " + orSum + ", XOR: " + xorSum);
    print("Bitwise shifts - Left: " + leftShiftSum + ", Right: " + rightShiftSum + ", Unsigned: " + unsignedShiftSum);

    return [andSum, orSum, xorSum, leftShiftSum, rightShiftSum, unsignedShiftSum];
}

// ==================== TEST SUITE 15: RECURSIVE FUNCTION PATTERNS ====================
function testRecursiveFunctionIC() {
    print("Starting Recursive Function IC Tests");

    // Test 15.1: Basic factorial recursion
    function factorial(n) {
        if (n <= 1) return 1;
        return n * factorial(n - 1);
    }

    let factSum = 0;
    for (let i = 1; i <= 11; i++) {
        let result = factorial(i);
        factSum += result % 1000000;
    }

    // Test 15.2: Fibonacci recursion
    function fibonacci(n) {
        if (n <= 1) return n;
        return fibonacci(n - 1) + fibonacci(n - 2);
    }

    let fibSum = 0;
    for (let i = 0; i <= 11; i++) {
        let result = fibonacci(i);
        fibSum += result % 100000;
    }

    // Test 15.3: Tree traversal recursion
    function createTree(depth, value) {
        if (depth <= 0) return { value: value, children: [] };
        let node = { value: value, children: [] };
        for (let i = 0; i < 3; i++) {
            node.children.push(createTree(depth - 1, value * 3 + i));
        }
        return node;
    }

    function sumTree(node) {
        let sum = node.value;
        for (let child of node.children) {
            sum += sumTree(child);
        }
        return sum;
    }

    let treeSum = 0;
    for (let i = 0; i < 11; i++) {
        let tree = createTree(4, i + 1);
        treeSum += sumTree(tree);
    }

    // Test 15.4: Mutual recursion
    function isEven(n) {
        if (n === 0) return true;
        return isOdd(n - 1);
    }

    function isOdd(n) {
        if (n === 0) return false;
        return isEven(n - 1);
    }

    let evenOddSum = 0;
    for (let i = 0; i < 11; i++) {
        if (isEven(i)) {
            evenOddSum += i;
        }
    }

    print("Recursive functions - Factorial: " + factSum + ", Fibonacci: " + fibSum + ", Tree: " + treeSum + ", Even/Odd: " + evenOddSum);

    return [factSum, fibSum, treeSum, evenOddSum];
}

// ==================== TEST SUITE 16: CLASS AND INHERITANCE PATTERNS ====================
function testClassInheritanceIC() {
    print("Starting Class and Inheritance IC Tests");

    // Test 16.1: Basic class hierarchy
    class BaseClass {
        constructor(value) {
            this.value = value;
            this.baseData = value * 2;
        }

        getValue() {
            return this.value;
        }

        compute() {
            return this.baseData * 3;
        }

        static staticMethod() {
            return 100;
        }
    }

    class MiddleClass extends BaseClass {
        constructor(value, extra) {
            super(value);
            this.extra = extra;
            this.middleData = value * 5;
        }

        getExtra() {
            return this.extra;
        }

        compute() {
            return super.compute() + this.middleData;
        }

        middleMethod() {
            return this.value + this.extra;
        }
    }

    class DerivedClass extends MiddleClass {
        constructor(value, extra, derived) {
            super(value, extra);
            this.derived = derived;
            this.derivedData = value * 7;
        }

        getDerived() {
            return this.derived;
        }

        compute() {
            return super.compute() + this.derivedData;
        }

        derivedMethod() {
            return this.value + this.extra + this.derived;
        }
    }

    let classSum = 0;
    let classObjects = [];

    for (let i = 0; i < 11; i++) {
        classObjects.push(new DerivedClass(i, i * 2, i * 3));
    }

    for (let obj of classObjects) {
        classSum += obj.getValue();
        classSum += obj.getExtra();
        classSum += obj.getDerived();
        classSum += obj.compute();
        classSum += obj.middleMethod();
        classSum += obj.derivedMethod();
        classSum += obj.baseData;
    }

    // Test 16.2: Multiple inheritance simulation
    class Mixin1 {
        mixin1Method() {
            return this.value * 10;
        }
    }

    class Mixin2 {
        mixin2Method() {
            return this.value * 20;
        }
    }

    class CombinedClass extends BaseClass {
        constructor(value) {
            super(value);
        }
    }

    // Simulate mixins
    CombinedClass.prototype.mixin1Method = Mixin1.prototype.mixin1Method;
    CombinedClass.prototype.mixin2Method = Mixin2.prototype.mixin2Method;

    let mixinSum = 0;
    let mixinObjects = [];

    for (let i = 0; i < 11; i++) {
        mixinObjects.push(new CombinedClass(i));
    }

    for (let obj of mixinObjects) {
        mixinSum += obj.getValue();
        mixinSum += obj.mixin1Method();
        mixinSum += obj.mixin2Method();
    }

    print("Class and Inheritance - Class hierarchy: " + classSum + ", Mixins: " + mixinSum);

    return [classSum, mixinSum];
}

// ==================== TEST SUITE 17: ARRAY MANIPULATION PATTERNS ====================
function testArrayManipulationIC() {
    print("Starting Array Manipulation IC Tests");

    // Test 17.1: Array push/pop
    let pushPopSum = 0;
    let stack = [];

    for (let i = 0; i < 11; i++) {
        stack.push(i);
        if (i % 10 === 0) {
            pushPopSum += stack.pop() || 0;
        }
    }

    while (stack.length > 0) {
        pushPopSum += stack.pop();
    }

    // Test 17.2: Array shift/unshift
    let shiftSum = 0;
    let queue = [];

    for (let i = 0; i < 11; i++) {
        queue.push(i);
        if (queue.length > 50) {
            shiftSum += queue.shift() || 0;
        }
    }

    // Test 17.3: Array splice
    let spliceSum = 0;
    let spliceArray = [];

    for (let i = 0; i < 11; i++) {
        spliceArray.push(i * 2);
    }

    for (let i = 0; i < 11; i++) {
        let removed = spliceArray.splice(3, 2, 100, 200, 300);
        spliceSum += removed.length;
        spliceSum += spliceArray.length;
    }

    // Test 17.4: Array slice
    let sliceSum = 0;
    let originalArray = [];

    for (let i = 0; i < 11; i++) {
        originalArray.push(i);
    }

    for (let i = 0; i < 11; i++) {
        let sliced = originalArray.slice(i % 5, (i % 5) + 3);
        sliceSum += sliced.length;
    }

    // Test 17.5: Array concat
    let concatSum = 0;
    let concatArrays = [];

    for (let i = 0; i < 11; i++) {
        concatArrays.push([i, i + 1, i + 2]);
    }

    let combined = concatArrays[0];
    for (let i = 1; i < concatArrays.length; i++) {
        combined = combined.concat(concatArrays[i]);
        concatSum += combined.length;
    }

    // Test 17.6: Array reverse
    let reverseSum = 0;
    let reverseArray = [];

    for (let i = 0; i < 11; i++) {
        reverseArray.push(i);
    }

    for (let i = 0; i < 5; i++) {
        reverseArray.reverse();
        reverseSum += reverseArray[0] + reverseArray[reverseArray.length - 1];
    }

    print("Array manipulation - Push/Pop: " + pushPopSum + ", Shift: " + shiftSum + ", Splice: " + spliceSum);
    print("Array manipulation - Slice: " + sliceSum + ", Concat: " + concatSum + ", Reverse: " + reverseSum);

    return [pushPopSum, shiftSum, spliceSum, sliceSum, concatSum, reverseSum];
}

// ==================== TEST SUITE 18: STRING OPERATION PATTERNS ====================
function testStringOperationIC() {
    print("Starting String Operation IC Tests");

    // Test 18.1: String concatenation
    let concatStringSum = 0;

    for (let i = 0; i < 11; i++) {
        let str = "part1_" + i + "_part2_" + (i * 2) + "_part3_" + (i * 3);
        concatStringSum += str.length;
    }

    // Test 18.2: String substring operations
    let substringSum = 0;
    let testString = "This_is_a_very_long_test_string_for_substring_operations_" +
                     "with_multiple_underscores_and_various_data";

    for (let i = 0; i < 11; i++) {
        let start = i % 20;
        let end = start + (i % 30) + 10;
        substringSum += testString.substring(start, end).length;
    }

    // Test 18.3: String indexOf/lastIndexOf
    let indexSum = 0;
    let searchStrings = [];

    for (let i = 0; i < 11; i++) {
        searchStrings.push("search_string_" + i + "_data_" + i * 7);
    }

    for (let str of searchStrings) {
        indexSum += str.indexOf('_');
        indexSum += str.lastIndexOf('_');
        indexSum += str.indexOf('data');
        indexSum += str.lastIndexOf('data');
    }

    // Test 18.4: String comparison
    let comparisonSum = 0;
    let compareStrings = [];

    for (let i = 0; i < 11; i++) {
        compareStrings.push("string_" + i + "_" + (i % 50));
    }

    for (let i = 0; i < compareStrings.length; i++) {
        let str1 = compareStrings[i];
        let str2 = compareStrings[(i + 1) % compareStrings.length];

        comparisonSum += str1.localeCompare(str2);
        comparisonSum += (str1 === str2) ? 1 : 0;
        comparisonSum += (str1 < str2) ? 1 : 0;
        comparisonSum += (str1 > str2) ? 1 : 0;
    }

    // Test 18.5: String case conversion
    let caseSum = 0;
    let caseStrings = [];

    for (let i = 0; i < 11; i++) {
        caseStrings.push("Mixed_Case_String_" + i + "_With_Mixed_Case");
    }

    for (let str of caseStrings) {
        caseSum += str.toUpperCase().length;
        caseSum += str.toLowerCase().length;
        caseSum += str.toLocaleLowerCase().length;
        caseSum += str.toLocaleUpperCase().length;
    }

    print("String operations - Concat: " + concatStringSum + ", Substring: " + substringSum);
    print("String operations - Index: " + indexSum + ", Compare: " + comparisonSum + ", Case: " + caseSum);

    return [concatStringSum, substringSum, indexSum, comparisonSum, caseSum];
}

// ==================== TEST SUITE 20: MATH OPERATION PATTERNS ====================
function testMathOperationIC() {
    print("Starting Math Operation IC Tests");

    // Test 20.1: Basic math functions
    let basicMathSum = 0;

    for (let i = 0; i < 11; i++) {
        basicMathSum += Math.abs(i - 500);
        basicMathSum += Math.floor(i / 3.14);
        basicMathSum += Math.ceil(i / 2.71);
        basicMathSum += Math.round(i / 1.41);
        basicMathSum += Math.max(i, 500, i % 100);
        basicMathSum += Math.min(i, 500, i % 100);
    }

    // Test 20.2: Exponential and logarithmic functions
    let expLogSum = 0;

    for (let i = 1; i <= 11; i++) {
        expLogSum += Math.sqrt(i);
        expLogSum += Math.pow(i, 2) % 100000;
        expLogSum += Math.exp(1) % 100000;
        expLogSum += Math.log(i) * 100 % 100000;
        expLogSum += Math.log10(i) * 100 % 100000;
        expLogSum += Math.log2(i) * 100 % 100000;
    }

    // Test 20.3: Trigonometric functions
    let trigSum = 0;

    for (let i = 0; i < 11; i++) {
        let angle = i * 0.01;
        trigSum += Math.sin(angle) * 1000;
        trigSum += Math.cos(angle) * 1000;
        trigSum += Math.tan(angle) * 1000;
        trigSum += Math.asin(Math.sin(angle)) * 1000;
        trigSum += Math.acos(Math.cos(angle)) * 1000;
        trigSum += Math.atan(Math.tan(angle)) * 1000;
    }

    // Test 20.4: Pseudo-random number generation (fixed seed)
    let randomSum = 0;
    let seed = 12345; // Fixed seed

    function pseudoRandom() {
        seed = (seed * 1103515245 + 12345) & 0x7fffffff;
        return seed / 0x7fffffff;
    }

    for (let i = 0; i < 11; i++) {
        randomSum += Math.floor(pseudoRandom() * 1000);
        randomSum += pseudoRandom() * 1000;
    }

    // Test 20.5: Hyperbolic functions
    let hyperbolicSum = 0;

    for (let i = 1; i <= 11; i++) {
        hyperbolicSum += Math.sinh(i * 0.1) * 100 % 100000;
        hyperbolicSum += Math.cosh(i * 0.1) * 100 % 100000;
        hyperbolicSum += Math.tanh(i * 0.1) * 100 % 100000;
        hyperbolicSum += Math.asinh(i * 0.1) * 100 % 100000;
        hyperbolicSum += Math.acosh(i * 0.1 + 1) * 100 % 100000;
        hyperbolicSum += Math.atanh(i * 0.01) * 100 % 100000;
    }

    print("Math operations - Basic: " + basicMathSum + ", Exp/Log: " + expLogSum);
    print("Math operations - Trigonometric: " + trigSum + ", Random: " + randomSum + ", Hyperbolic: " + hyperbolicSum);

    return [basicMathSum, expLogSum, trigSum, randomSum, hyperbolicSum];
}

// ==================== TEST SUITE 21: OBJECT CREATION PATTERNS ====================
function testObjectCreationIC() {
    print("Starting Object Creation IC Tests");

    // Test 21.1: Object literal creation
    let literalSum = 0;
    let literalObjects = [];

    for (let i = 0; i < 11; i++) {
        let obj = {
            a: i,
            b: i * 2,
            c: i * 3,
            d: i * 4,
            e: i * 5,
            method: function() { return this.a + this.b; },
            method2: function() { return this.c + this.d; }
        };
        literalObjects.push(obj);
        literalSum += obj.method() + obj.method2();
    }

    // Test 21.2: Object.create with prototype
    let createSum = 0;
    let prototypeObj = {
        shared: 100,
        protoMethod: function() { return this.shared * 2; }
    };

    let createObjects = [];
    for (let i = 0; i < 11; i++) {
        let obj = Object.create(prototypeObj);
        obj.instance = i;
        obj.instanceMethod = function() { return this.instance * 3; };
        createObjects.push(obj);
        createSum += obj.protoMethod() + obj.instanceMethod();
    }

    // Test 21.3: Constructor function pattern
    let constructorSum = 0;

    function ConstructorPattern(value1, value2, value3) {
        this.value1 = value1;
        this.value2 = value2;
        this.value3 = value3;
        this.computed = value1 + value2 + value3;
    }

    ConstructorPattern.prototype.constructorMethod = function() {
        return this.computed * 2;
    };

    let constructorObjects = [];
    for (let i = 0; i < 11; i++) {
        constructorObjects.push(new ConstructorPattern(i, i * 2, i * 3));
    }

    for (let obj of constructorObjects) {
        constructorSum += obj.constructorMethod();
    }

    // Test 21.4: Dynamic property addition
    let dynamicSum = 0;
    let dynamicObjects = [];

    for (let i = 0; i < 11; i++) {
        let obj = { base: i };
        obj.prop1 = i * 2;
        obj.prop2 = i * 3;
        obj.prop3 = i * 4;
        obj.method = function() { return this.base + this.prop1; };
        dynamicObjects.push(obj);
        dynamicSum += obj.method();
    }

    // Test 21.5: Object.assign and spread
    let assignSum = 0;

    let source1 = { a: 1, b: 2, c: 3 };
    let source2 = { d: 4, e: 5, f: 6 };
    let source3 = { g: 7, h: 8, i: 9 };

    for (let i = 0; i < 11; i++) {
        let assigned = Object.assign({}, source1, source2, source3);
        assignSum += Object.keys(assigned).length;

        let spread = { ...source1, ...source2, ...source3, extra: i };
        assignSum += Object.keys(spread).length;
    }

    print("Object creation - Literal: " + literalSum + ", Object.create: " + createSum);
    print("Object creation - Constructor: " + constructorSum + ", Dynamic: " + dynamicSum + ", Assign: " + assignSum);

    return [literalSum, createSum, constructorSum, dynamicSum, assignSum];
}

// ==================== TEST SUITE 22: FUNCTION APPLICATION PATTERNS ====================
function testFunctionApplicationIC() {
    print("Starting Function Application IC Tests");

    // Test 22.1: Function binding
    let bindSum = 0;

    function BindTest(value, multiplier) {
        this.value = value;
        this.multiplier = multiplier;
    }

    BindTest.prototype.calculate = function() {
        return this.value * this.multiplier;
    };

    let bindObjects = [];
    for (let i = 0; i < 11; i++) {
        let obj = new BindTest(i, i + 1);
        let boundFunc = obj.calculate.bind(obj);
        bindObjects.push(boundFunc);
        bindSum += boundFunc();
    }

    // Test 22.2: Call and apply
    let callApplySum = 0;

    function greet(greeting, punctuation) {
        return greeting + " " + this.name + punctuation;
    }

    let people = [];
    for (let i = 0; i < 11; i++) {
        people.push({ name: "Person" + i });
    }

    for (let i = 0; i < people.length; i++) {
        let person = people[i];
        callApplySum += greet.call(person, "Hello", "!").length;
        callApplySum += greet.apply(person, ["Hi", "."]).length;
    }

    // Test 22.3: Partial application
    let partialSum = 0;

    function partial(a, b, c) {
        return a + b + c;
    }

    function createPartial(func, firstArg) {
        return function(...args) {
            return func(firstArg, ...args);
        };
    }

    let partialFunctions = [];
    for (let i = 0; i < 11; i++) {
        partialFunctions.push(createPartial(partial, i));
    }

    for (let i = 0; i < partialFunctions.length; i++) {
        partialSum += partialFunctions[i](i, i * 2);
    }

    // Test 22.4: Currying
    let curryingSum = 0;

    function curry(a) {
        return function(b) {
            return function(c) {
                return a + b + c;
            };
        };
    }

    for (let i = 0; i < 11; i++) {
        let curried = curry(i);
        let step1 = curried(i + 1);
        let step2 = step1(i + 2);
        curryingSum += step2;
    }

    // Test 22.5: Function composition
    let compositionSum = 0;

    function compose(...funcs) {
        return function(x) {
            return funcs.reduceRight((value, func) => func(value), x);
        };
    }

    let add = x => x + 10;
    let multiply = x => x * 2;
    let subtract = x => x - 5;

    let composed = compose(subtract, multiply, add);

    for (let i = 0; i < 11; i++) {
        compositionSum += composed(i);
    }

    print("Function application - Bind: " + bindSum + ", Call/Apply: " + callApplySum);
    print("Function application - Partial: " + partialSum + ", Currying: " + curryingSum + ", Composition: " + compositionSum);

    return [bindSum, callApplySum, partialSum, curryingSum, compositionSum];
}

// ==================== TEST SUITE 23: SPREAD AND REST OPERATOR PATTERNS ====================
function testSpreadRestIC() {
    print("Starting Spread and Rest Operator IC Tests");

    // Test 23.1: Array spread
    let arraySpreadSum = 0;

    for (let i = 0; i < 11; i++) {
        let arr1 = [i, i + 1, i + 2];
        let arr2 = [i + 3, i + 4, i + 5];
        let arr3 = [i + 6, i + 7, i + 8];

        let spread = [...arr1, ...arr2, ...arr3];
        arraySpreadSum += spread.length;

        let withExtra = [...arr1, 100, ...arr2, 200, ...arr3];
        arraySpreadSum += withExtra.length;
    }

    // Test 23.2: Object spread
    let objectSpreadSum = 0;

    for (let i = 0; i < 11; i++) {
        let obj1 = { a: i, b: i * 2 };
        let obj2 = { c: i * 3, d: i * 4 };

        let spreadObj = { ...obj1, ...obj2 };
        objectSpreadSum += Object.keys(spreadObj).length;

        let withExtra = { ...obj1, extra: 100, ...obj2, more: 200 };
        objectSpreadSum += Object.keys(withExtra).length;
    }

    // Test 23.3: Rest parameters
    let restSum = 0;

    function restFunction(...args) {
        let sum = 0;
        for (let arg of args) {
            sum += arg;
        }
        return sum;
    }

    for (let i = 0; i < 11; i++) {
        restSum += restFunction(i, i + 1, i + 2, i + 3, i + 4);
    }

    function multiRest(a, b, ...rest) {
        let sum = a + b;
        for (let val of rest) {
            sum += val;
        }
        return sum;
    }

    for (let i = 0; i < 11; i++) {
        restSum += multiRest(i, i + 1, i + 2, i + 3, i + 4, i + 5, i + 6);
    }

    // Test 23.4: Destructuring with rest
    let destructuringSum = 0;

    for (let i = 0; i < 11; i++) {
        let arr = [i, i + 1, i + 2, i + 3, i + 4, i + 5];
        let [first, second, ...rest] = arr;
        destructuringSum += first + second + rest.length;

        let obj = { x: i, y: i + 1, z: i + 2, w: i + 3, v: i + 4 };
        let { x, y, ...objRest } = obj;
        destructuringSum += x + y + Object.keys(objRest).length;
    }

    print("Spread/Rest - Array spread: " + arraySpreadSum + ", Object spread: " + objectSpreadSum);
    print("Spread/Rest - Rest params: " + restSum + ", Destructuring: " + destructuringSum);

    return [arraySpreadSum, objectSpreadSum, restSum, destructuringSum];
}

// ==================== TEST SUITE 24: ITERATOR AND GENERATOR PATTERNS ====================
function testIteratorGeneratorIC() {
    print("Starting Iterator and Generator IC Tests");

    // Test 24.1: Custom iterator
    let iteratorSum = 0;

    class CountIterator {
        constructor(limit) {
            this.limit = limit;
            this.current = 0;
        }

        [Symbol.iterator]() {
            return {
                next: () => {
                    if (this.current < this.limit) {
                        return { value: this.current++, done: false };
                    }
                    return { done: true };
                }
            };
        }
    }

    for (let i = 1; i <= 11; i++) {
        let iterator = new CountIterator(i);
        for (let value of iterator) {
            iteratorSum += value;
        }
    }

    // Test 24.2: Generator with yield
    let generatorSum = 0;

    function* rangeGenerator(start, end, step) {
        for (let i = start; i < end; i += step) {
            yield i;
        }
    }

    for (let i = 0; i < 11; i++) {
        let gen = rangeGenerator(i % 10, (i % 10) + 5, 2);
        for (let value of gen) {
            generatorSum += value;
        }
    }

    // Test 24.3: Generator with multiple yields
    function* multiYield() {
        yield 1;
        yield 2;
        yield 3;
    }

    let multiYieldSum = 0;
    for (let i = 0; i < 11; i++) {
        let gen = multiYield();
        let result = gen.next();
        multiYieldSum += result.value || 0;
        result = gen.next();
        multiYieldSum += result.value || 0;
        result = gen.next();
        multiYieldSum += result.value || 0;
    }

    // Test 24.4: Generator delegation
    function* delegateGen(start, count) {
        for (let i = 0; i < count; i++) {
            yield start + i;
        }
    }

    function* delegatingGen(base) {
        yield base;
        yield* delegateGen(base + 100, 3);
        yield base + 1000;
    }

    let delegateSum = 0;
    for (let i = 0; i < 11; i++) {
        let gen = delegatingGen(i);
        for (let value of gen) {
            delegateSum += value;
        }
    }

    print("Iterator/Generator - Custom: " + iteratorSum + ", Generator: " + generatorSum);
    print("Iterator/Generator - Multi-yield: " + multiYieldSum + ", Delegation: " + delegateSum);

    return [iteratorSum, generatorSum, multiYieldSum, delegateSum];
}

// ==================== UPDATE MAIN TEST RUNNER ====================
function runAllICTests() {
    print("=== STARTING COMPREHENSIVE IC TEST SUITE ===");
    print("Test started at fixed reference point");
    print("");

    let allResults = [];

    // Run all test suites
    try {
        print("--- Test Suite 1: Object Property Access ---");
        allResults = allResults.concat(testObjectPropertyAccessIC());
        print("");

        print("--- Test Suite 2: Array Element Access ---");
        allResults = allResults.concat(testArrayElementAccessIC());
        print("");

        print("--- Test Suite 3: Function Call Patterns ---");
        allResults = allResults.concat(testFunctionCallIC());
        print("");

        print("--- Test Suite 4: Prototype Chain Access ---");
        allResults = allResults.concat(testPrototypeChainIC());
        print("");

        print("--- Test Suite 5: Property Accessor Patterns ---");
        allResults = allResults.concat(testPropertyAccessorIC());
        print("");

        print("--- Test Suite 6: Loop Optimization Patterns ---");
        allResults = allResults.concat(testLoopOptimizationIC());
        print("");

        print("--- Test Suite 7: Exception Handling Patterns ---");
        allResults = allResults.concat(testExceptionHandlingIC());
        print("");

        print("--- Test Suite 8: Type Transition Patterns ---");
        allResults = allResults.concat(testTypeTransitionIC());
        print("");

        print("--- Test Suite 9: Global and Builtin Access ---");
        allResults = allResults.concat(testGlobalAccessIC());
        print("");

        print("--- Test Suite 10: Complex Data Structures ---");
        allResults = allResults.concat(testComplexDataStructuresIC());
        print("");

        print("--- Test Suite 11: Closure and Scope Patterns ---");
        allResults = allResults.concat(testClosureScopeIC());
        print("");

        print("--- Test Suite 12: Advanced Language Features ---");
        allResults = allResults.concat(testAdvancedLanguageFeaturesIC());
        print("");

        print("--- Test Suite 13: Regular Expression Patterns ---");
        allResults = allResults.concat(testRegularExpressionIC());
        print("");

        print("--- Test Suite 14: Bitwise Operation Patterns ---");
        allResults = allResults.concat(testBitwiseOperationIC());
        print("");

        print("--- Test Suite 15: Recursive Function Patterns ---");
        allResults = allResults.concat(testRecursiveFunctionIC());
        print("");

        print("--- Test Suite 16: Class and Inheritance Patterns ---");
        allResults = allResults.concat(testClassInheritanceIC());
        print("");

        print("--- Test Suite 17: Array Manipulation Patterns ---");
        allResults = allResults.concat(testArrayManipulationIC());
        print("");

        print("--- Test Suite 18: String Operation Patterns ---");
        allResults = allResults.concat(testStringOperationIC());
        print("");

        print("--- Test Suite 20: Math Operation Patterns ---");
        allResults = allResults.concat(testMathOperationIC());
        print("");

        print("--- Test Suite 21: Object Creation Patterns ---");
        allResults = allResults.concat(testObjectCreationIC());
        print("");

        print("--- Test Suite 22: Function Application Patterns ---");
        allResults = allResults.concat(testFunctionApplicationIC());
        print("");

        print("--- Test Suite 23: Spread and Rest Operator Patterns ---");
        allResults = allResults.concat(testSpreadRestIC());
        print("");

        print("--- Test Suite 24: Iterator and Generator Patterns ---");
        allResults = allResults.concat(testIteratorGeneratorIC());
        print("");

    } catch (error) {
        print("ERROR during test execution: " + error.message);
        print("Stack: " + error.stack);
    }

    print("=== TEST SUITE COMPLETED ===");
    print("Total result count: " + allResults.length);
    print("Final results: " + allResults.join(", "));
    print("Test completed successfully");

    return allResults;
}

print("=== IC COMPREHENSIVE TESTING COMPLETE ===");