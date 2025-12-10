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

declare function print(arg: any): string;
declare const ArkTools: {
    jitCompileAsync(args: any): boolean;
    waitJitCompileFinish(args: any): boolean;
};

// ==================== Static Members ====================

// Static property
class StaticProp {
    static count: number = 0;
    static name2: string = "StaticProp";

    constructor() {
        StaticProp.count++;
    }

    static getCount(): number {
        return StaticProp.count;
    }
}

function testStaticProp(): string {
    StaticProp.count = 0;
    new StaticProp();
    new StaticProp();
    new StaticProp();
    return `${StaticProp.count},${StaticProp.name2}`;
}

// Static method
class StaticMethod {
    static add(a: number, b: number): number {
        return a + b;
    }

    static multiply(a: number, b: number): number {
        return a * b;
    }

    static calculate(a: number, b: number): string {
        return `${this.add(a, b)},${this.multiply(a, b)}`;
    }
}

function testStaticMethod(): string {
    return StaticMethod.calculate(3, 4);
}

// Static with inheritance
class StaticBase {
    static baseValue: number = 100;
    static getBaseValue(): number {
        return this.baseValue;
    }
}

class StaticDerived extends StaticBase {
    static derivedValue: number = 200;
    static getBothValues(): string {
        return `${this.baseValue},${this.derivedValue}`;
    }
}

function testStaticInherit(): string {
    return `${StaticDerived.getBaseValue()},${StaticDerived.getBothValues()}`;
}

// ==================== Private Members ====================

// Private fields
class PrivateFields {
    #privateValue: number;
    #privateString: string;
    publicValue: number;

    constructor(val: number, str: string) {
        this.#privateValue = val;
        this.#privateString = str;
        this.publicValue = val * 2;
    }

    getPrivateValue(): number {
        return this.#privateValue;
    }

    getPrivateString(): string {
        return this.#privateString;
    }

    setPrivateValue(val: number): void {
        this.#privateValue = val;
    }
}

function testPrivateFields(): string {
    const obj = new PrivateFields(10, "secret");
    const v1 = obj.getPrivateValue();
    obj.setPrivateValue(20);
    const v2 = obj.getPrivateValue();
    return `${v1},${v2},${obj.getPrivateString()},${obj.publicValue}`;
}

// Private methods
class PrivateMethods {
    #value: number;

    constructor(val: number) {
        this.#value = val;
    }

    #double(): number {
        return this.#value * 2;
    }

    #triple(): number {
        return this.#value * 3;
    }

    calculate(): string {
        return `${this.#double()},${this.#triple()}`;
    }
}

function testPrivateMethods(): string {
    const obj = new PrivateMethods(5);
    return obj.calculate();
}

// Private static
class PrivateStatic {
    static #secret: string = "hidden";
    static #counter: number = 0;

    static #increment(): void {
        this.#counter++;
    }

    static resetCounter(): void {
        this.#counter = 0;
    }

    static getSecret(): string {
        this.#increment();
        return `${this.#secret}:${this.#counter}`;
    }
}

function testPrivateStatic(): string {
    PrivateStatic.resetCounter();
    const r1 = PrivateStatic.getSecret();
    const r2 = PrivateStatic.getSecret();
    return `${r1},${r2}`;
}

// ==================== Inheritance ====================

// Basic inheritance
class Animal {
    name: string;
    constructor(name: string) {
        this.name = name;
    }
    speak(): string {
        return `${this.name} makes a sound`;
    }
}

class Dog extends Animal {
    breed: string;
    constructor(name: string, breed: string) {
        super(name);
        this.breed = breed;
    }
    speak(): string {
        return `${this.name} barks`;
    }
    getInfo(): string {
        return `${this.name} is a ${this.breed}`;
    }
}

function testBasicInherit(): string {
    const dog = new Dog("Rex", "German Shepherd");
    return `${dog.speak()},${dog.getInfo()}`;
}

// Multi-level inheritance
class Vehicle {
    speed: number = 0;
    accelerate(amount: number): void {
        this.speed += amount;
    }
    getSpeed(): number {
        return this.speed;
    }
}

class Car extends Vehicle {
    wheels: number = 4;
    honk(): string {
        return "Beep!";
    }
}

class SportsCar extends Car {
    turbo: boolean = true;
    accelerate(amount: number): void {
        super.accelerate(amount * 2);
    }
    getTurboStatus(): string {
        return this.turbo ? "Turbo ON" : "Turbo OFF";
    }
}

function testMultiLevelInherit(): string {
    const car = new SportsCar();
    car.accelerate(50);
    return `${car.getSpeed()},${car.wheels},${car.honk()},${car.getTurboStatus()}`;
}

// Super calls
class Parent {
    value: number;
    constructor(val: number) {
        this.value = val;
    }
    getValue(): number {
        return this.value;
    }
    describe(): string {
        return `Parent:${this.value}`;
    }
}

class Child extends Parent {
    extra: number;
    constructor(val: number, extra: number) {
        super(val);
        this.extra = extra;
    }
    getValue(): number {
        return super.getValue() + this.extra;
    }
    describe(): string {
        return `${super.describe()},Child:${this.extra}`;
    }
}

function testSuperCalls(): string {
    const child = new Child(10, 5);
    return `${child.getValue()},${child.describe()}`;
}

// ==================== Accessors ====================

// Getter and setter
class Rectangle {
    #width: number;
    #height: number;

    constructor(width: number, height: number) {
        this.#width = width;
        this.#height = height;
    }

    get width(): number {
        return this.#width;
    }

    set width(value: number) {
        if (value > 0) this.#width = value;
    }

    get height(): number {
        return this.#height;
    }

    set height(value: number) {
        if (value > 0) this.#height = value;
    }

    get area(): number {
        return this.#width * this.#height;
    }

    get perimeter(): number {
        return 2 * (this.#width + this.#height);
    }
}

function testAccessors(): string {
    const rect = new Rectangle(10, 5);
    const area1 = rect.area;
    rect.width = 20;
    const area2 = rect.area;
    return `${area1},${area2},${rect.perimeter}`;
}

// Computed property name accessor
class ComputedAccessor {
    #data: { [key: string]: number } = {};

    get ['computed_prop'](): number {
        return this.#data['computed_prop'] || 0;
    }

    set ['computed_prop'](value: number) {
        this.#data['computed_prop'] = value;
    }
}

function testComputedAccessor(): string {
    const obj = new ComputedAccessor();
    obj['computed_prop'] = 42;
    return `${obj['computed_prop']}`;
}

// ==================== Mixin Pattern ====================

// Mixin functions
type Constructor<T = {}> = new (...args: any[]) => T;

function Timestamped<TBase extends Constructor>(Base: TBase) {
    return class extends Base {
        timestamp = Date.now();
        getTimestamp(): number {
            return this.timestamp;
        }
    };
}

function Named<TBase extends Constructor>(Base: TBase) {
    return class extends Base {
        _name: string = "";
        setName(name: string): void {
            this._name = name;
        }
        getName(): string {
            return this._name;
        }
    };
}

class BaseClass {
    id: number;
    constructor(id: number) {
        this.id = id;
    }
}

const MixedClass = Named(Timestamped(BaseClass));

function testMixin(): string {
    const obj = new MixedClass(123);
    obj.setName("TestMixin");
    return `${obj.id},${obj.getName()},${obj.getTimestamp() > 0}`;
}

// ==================== Abstract-like Pattern ====================

// Base class with required override
class Shape {
    name: string;
    constructor(name: string) {
        this.name = name;
    }
    area(): number {
        throw new Error("Must override");
    }
    describe(): string {
        return `${this.name}: area=${this.area()}`;
    }
}

class Circle extends Shape {
    radius: number;
    constructor(radius: number) {
        super("Circle");
        this.radius = radius;
    }
    area(): number {
        return Math.PI * this.radius * this.radius;
    }
}

class Square extends Shape {
    side: number;
    constructor(side: number) {
        super("Square");
        this.side = side;
    }
    area(): number {
        return this.side * this.side;
    }
}

function testAbstractPattern(): string {
    const circle = new Circle(5);
    const square = new Square(4);
    return `${circle.area().toFixed(2)},${square.area()}`;
}

// ==================== Instance Checks ====================

function testInstanceOf(): string {
    const dog = new Dog("Max", "Labrador");
    const car = new SportsCar();
    return `${dog instanceof Animal},${dog instanceof Dog},${car instanceof Vehicle},${car instanceof SportsCar}`;
}

// Method chaining
class Builder {
    #value: string = "";

    append(str: string): this {
        this.#value += str;
        return this;
    }

    prepend(str: string): this {
        this.#value = str + this.#value;
        return this;
    }

    build(): string {
        return this.#value;
    }
}

function testMethodChaining(): string {
    const result = new Builder()
        .append("Hello")
        .append(" ")
        .append("World")
        .prepend(">>> ")
        .build();
    return result;
}

// Warmup
for (let i = 0; i < 20; i++) {
    testStaticProp();
    testStaticMethod();
    testStaticInherit();
    testPrivateFields();
    testPrivateMethods();
    testPrivateStatic();
    testBasicInherit();
    testMultiLevelInherit();
    testSuperCalls();
    testAccessors();
    testComputedAccessor();
    testMixin();
    testAbstractPattern();
    testInstanceOf();
    testMethodChaining();
}

// JIT compile
ArkTools.jitCompileAsync(testStaticProp);
ArkTools.jitCompileAsync(testStaticMethod);
ArkTools.jitCompileAsync(testStaticInherit);
ArkTools.jitCompileAsync(testPrivateFields);
ArkTools.jitCompileAsync(testPrivateMethods);
ArkTools.jitCompileAsync(testPrivateStatic);
ArkTools.jitCompileAsync(testBasicInherit);
ArkTools.jitCompileAsync(testMultiLevelInherit);
ArkTools.jitCompileAsync(testSuperCalls);
ArkTools.jitCompileAsync(testAccessors);
ArkTools.jitCompileAsync(testComputedAccessor);
ArkTools.jitCompileAsync(testMixin);
ArkTools.jitCompileAsync(testAbstractPattern);
ArkTools.jitCompileAsync(testInstanceOf);
ArkTools.jitCompileAsync(testMethodChaining);

print("compile testStaticProp: " + ArkTools.waitJitCompileFinish(testStaticProp));
print("compile testStaticMethod: " + ArkTools.waitJitCompileFinish(testStaticMethod));
print("compile testStaticInherit: " + ArkTools.waitJitCompileFinish(testStaticInherit));
print("compile testPrivateFields: " + ArkTools.waitJitCompileFinish(testPrivateFields));
print("compile testPrivateMethods: " + ArkTools.waitJitCompileFinish(testPrivateMethods));
print("compile testPrivateStatic: " + ArkTools.waitJitCompileFinish(testPrivateStatic));
print("compile testBasicInherit: " + ArkTools.waitJitCompileFinish(testBasicInherit));
print("compile testMultiLevelInherit: " + ArkTools.waitJitCompileFinish(testMultiLevelInherit));
print("compile testSuperCalls: " + ArkTools.waitJitCompileFinish(testSuperCalls));
print("compile testAccessors: " + ArkTools.waitJitCompileFinish(testAccessors));
print("compile testComputedAccessor: " + ArkTools.waitJitCompileFinish(testComputedAccessor));
print("compile testMixin: " + ArkTools.waitJitCompileFinish(testMixin));
print("compile testAbstractPattern: " + ArkTools.waitJitCompileFinish(testAbstractPattern));
print("compile testInstanceOf: " + ArkTools.waitJitCompileFinish(testInstanceOf));
print("compile testMethodChaining: " + ArkTools.waitJitCompileFinish(testMethodChaining));

// Verify
print("testStaticProp: " + testStaticProp());
print("testStaticMethod: " + testStaticMethod());
print("testStaticInherit: " + testStaticInherit());
print("testPrivateFields: " + testPrivateFields());
print("testPrivateMethods: " + testPrivateMethods());
print("testPrivateStatic: " + testPrivateStatic());
print("testBasicInherit: " + testBasicInherit());
print("testMultiLevelInherit: " + testMultiLevelInherit());
print("testSuperCalls: " + testSuperCalls());
print("testAccessors: " + testAccessors());
print("testComputedAccessor: " + testComputedAccessor());
print("testMixin: " + testMixin());
print("testAbstractPattern: " + testAbstractPattern());
print("testInstanceOf: " + testInstanceOf());
print("testMethodChaining: " + testMethodChaining());
