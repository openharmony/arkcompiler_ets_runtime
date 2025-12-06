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

// ==================== Proxy Basic ====================

function testProxyGet(): string {
    const target = { a: 1, b: 2 };
    const handler = {
        get(obj: any, prop: string) {
            return prop in obj ? obj[prop] : 'default';
        }
    };
    const proxy = new Proxy(target, handler);
    return `${proxy.a},${proxy.b},${proxy.c}`;
}

function testProxySet(): string {
    const target: Record<string, number> = { a: 1 };
    const handler = {
        set(obj: any, prop: string, value: number) {
            obj[prop] = value * 2;
            return true;
        }
    };
    const proxy = new Proxy(target, handler);
    proxy.b = 5;
    proxy.c = 10;
    return `${proxy.a},${proxy.b},${proxy.c}`;
}

function testProxyHas(): string {
    const target = { a: 1, b: 2 };
    const handler = {
        has(obj: any, prop: string) {
            return prop !== 'hidden' && prop in obj;
        }
    };
    const proxy = new Proxy(target, handler);
    return `${'a' in proxy},${'b' in proxy},${'hidden' in proxy}`;
}

function testProxyDeleteProperty(): string {
    const target: Record<string, number> = { a: 1, b: 2, c: 3 };
    let deletedProp = '';
    const handler = {
        deleteProperty(obj: any, prop: string) {
            deletedProp = prop;
            delete obj[prop];
            return true;
        }
    };
    const proxy = new Proxy(target, handler);
    delete proxy.b;
    return `${deletedProp},${'a' in proxy},${'b' in proxy},${'c' in proxy}`;
}

function testProxyOwnKeys(): string {
    const target = { a: 1, b: 2, _private: 3 };
    const handler = {
        ownKeys(obj: any) {
            return Object.keys(obj).filter(key => !key.startsWith('_'));
        }
    };
    const proxy = new Proxy(target, handler);
    return Object.keys(proxy).join(',');
}

function testProxyGetOwnPropertyDescriptor(): string {
    const target = { a: 1 };
    const handler = {
        getOwnPropertyDescriptor(obj: any, prop: string) {
            return {
                value: obj[prop],
                writable: true,
                enumerable: true,
                configurable: true
            };
        }
    };
    const proxy = new Proxy(target, handler);
    const desc = Object.getOwnPropertyDescriptor(proxy, 'a');
    return `${desc?.value},${desc?.writable},${desc?.enumerable}`;
}

// ==================== Proxy Apply and Construct ====================

function testProxyApply(): string {
    const target = function(a: number, b: number) { return a + b; };
    const handler = {
        apply(fn: Function, thisArg: any, args: any[]) {
            return fn.apply(thisArg, args) * 10;
        }
    };
    const proxy = new Proxy(target, handler);
    return `${proxy(2, 3)}`;
}

function testProxyConstruct(): string {
    class Target {
        value: number;
        constructor(value: number) {
            this.value = value;
        }
    }
    const handler = {
        construct(Target: any, args: any[]) {
            return new Target(args[0] * 2);
        }
    };
    const ProxyClass = new Proxy(Target, handler);
    const instance = new ProxyClass(5);
    return `${instance.value}`;
}

// ==================== Proxy Revocable ====================

function testProxyRevocable(): string {
    const target = { a: 1, b: 2 };
    const { proxy, revoke } = Proxy.revocable(target, {});
    const before = proxy.a;
    revoke();
    let afterError = false;
    try {
        const _ = proxy.a;
    } catch (e) {
        afterError = true;
    }
    return `${before},${afterError}`;
}

// ==================== Proxy Validation ====================

function testProxyValidation(): string {
    const target: Record<string, any> = { name: 'test', age: 25 };
    const handler = {
        set(obj: any, prop: string, value: any) {
            if (prop === 'age' && (typeof value !== 'number' || value < 0)) {
                return false;
            }
            obj[prop] = value;
            return true;
        }
    };
    const proxy = new Proxy(target, handler);
    proxy.age = 30;
    const validSet = proxy.age === 30;
    try {
        proxy.age = -5;
    } catch (e) {}
    return `${validSet},${proxy.age}`;
}

function testProxyLogging(): string {
    const target = { a: 1, b: 2 };
    const logs: string[] = [];
    const handler = {
        get(obj: any, prop: string) {
            logs.push(`get:${prop}`);
            return obj[prop];
        },
        set(obj: any, prop: string, value: any) {
            logs.push(`set:${prop}`);
            obj[prop] = value;
            return true;
        }
    };
    const proxy = new Proxy(target, handler);
    const _ = proxy.a;
    proxy.b = 5;
    return logs.join(',');
}

// ==================== Reflect Basic ====================

function testReflectGet(): string {
    const obj = { a: 1, b: 2 };
    return `${Reflect.get(obj, 'a')},${Reflect.get(obj, 'b')},${Reflect.get(obj, 'c')}`;
}

function testReflectSet(): string {
    const obj: Record<string, number> = { a: 1 };
    const result = Reflect.set(obj, 'b', 2);
    return `${result},${obj.b}`;
}

function testReflectHas(): string {
    const obj = { a: 1, b: 2 };
    return `${Reflect.has(obj, 'a')},${Reflect.has(obj, 'c')}`;
}

function testReflectDeleteProperty(): string {
    const obj: Record<string, number> = { a: 1, b: 2 };
    const result = Reflect.deleteProperty(obj, 'a');
    return `${result},${Reflect.has(obj, 'a')},${Reflect.has(obj, 'b')}`;
}

function testReflectOwnKeys(): string {
    const obj = { a: 1, b: 2, c: 3 };
    const keys = Reflect.ownKeys(obj);
    return keys.join(',');
}

function testReflectGetPrototypeOf(): string {
    const proto = { x: 1 };
    const obj = Object.create(proto);
    return `${Reflect.getPrototypeOf(obj) === proto}`;
}

function testReflectSetPrototypeOf(): string {
    const obj = {};
    const proto = { x: 10 };
    const result = Reflect.setPrototypeOf(obj, proto);
    return `${result},${Reflect.getPrototypeOf(obj) === proto}`;
}

function testReflectIsExtensible(): string {
    const obj = { a: 1 };
    const before = Reflect.isExtensible(obj);
    Object.preventExtensions(obj);
    const after = Reflect.isExtensible(obj);
    return `${before},${after}`;
}

function testReflectPreventExtensions(): string {
    const obj = { a: 1 };
    const result = Reflect.preventExtensions(obj);
    return `${result},${Reflect.isExtensible(obj)}`;
}

function testReflectGetOwnPropertyDescriptor(): string {
    const obj = { a: 1 };
    const desc = Reflect.getOwnPropertyDescriptor(obj, 'a');
    return `${desc?.value},${desc?.writable},${desc?.enumerable}`;
}

function testReflectDefineProperty(): string {
    const obj: Record<string, number> = {};
    const result = Reflect.defineProperty(obj, 'a', {
        value: 42,
        writable: true,
        enumerable: true,
        configurable: true
    });
    return `${result},${obj.a}`;
}

// ==================== Reflect Apply and Construct ====================

function testReflectApply(): string {
    function sum(a: number, b: number, c: number) {
        return a + b + c;
    }
    return `${Reflect.apply(sum, null, [1, 2, 3])}`;
}

function testReflectConstruct(): string {
    class Person {
        name: string;
        age: number;
        constructor(name: string, age: number) {
            this.name = name;
            this.age = age;
        }
    }
    const person = Reflect.construct(Person, ['Alice', 30]);
    return `${person.name},${person.age}`;
}

function testReflectConstructNewTarget(): string {
    class Animal {
        type: string;
        constructor() {
            this.type = 'animal';
        }
    }
    class Dog {
        type: string;
        constructor() {
            this.type = 'dog';
        }
    }
    const instance = Reflect.construct(Animal, [], Dog);
    return `${instance instanceof Dog}`;
}

// ==================== Proxy with Reflect ====================

function testProxyWithReflect(): string {
    const target = { a: 1, b: 2 };
    const handler = {
        get(obj: any, prop: string, receiver: any) {
            const value = Reflect.get(obj, prop, receiver);
            return typeof value === 'number' ? value * 2 : value;
        }
    };
    const proxy = new Proxy(target, handler);
    return `${proxy.a},${proxy.b}`;
}

function testProxyForwarding(): string {
    const target: Record<string, any> = { a: 1, b: 2 };
    const handler = {
        get(obj: any, prop: string, receiver: any) {
            return Reflect.get(obj, prop, receiver);
        },
        set(obj: any, prop: string, value: any, receiver: any) {
            return Reflect.set(obj, prop, value, receiver);
        },
        has(obj: any, prop: string) {
            return Reflect.has(obj, prop);
        }
    };
    const proxy = new Proxy(target, handler);
    proxy.c = 3;
    return `${proxy.a},${'b' in proxy},${proxy.c}`;
}

// ==================== Advanced Proxy Patterns ====================

function testProxyNegativeArray(): string {
    const arr = [1, 2, 3, 4, 5];
    const handler = {
        get(obj: number[], prop: string) {
            const index = Number(prop);
            if (!isNaN(index) && index < 0) {
                return obj[obj.length + index];
            }
            return Reflect.get(obj, prop);
        }
    };
    const proxy = new Proxy(arr, handler);
    return `${proxy[-1]},${proxy[-2]},${proxy[0]}`;
}

function testProxyDefaultValues(): string {
    const defaults = { a: 1, b: 2, c: 3 };
    const target: Record<string, number> = { a: 10 };
    const handler = {
        get(obj: any, prop: string) {
            return prop in obj ? obj[prop] : defaults[prop as keyof typeof defaults];
        }
    };
    const proxy = new Proxy(target, handler);
    return `${proxy.a},${proxy.b},${proxy.c}`;
}

function testProxyReadOnly(): string {
    const target = { a: 1, b: 2 };
    const handler = {
        set() { return false; },
        deleteProperty() { return false; }
    };
    const proxy = new Proxy(target, handler);
    let setFailed = false;
    let deleteFailed = false;
    try {
        proxy.a = 10;
    } catch (e) {
        setFailed = true;
    }
    try {
        delete (proxy as any).a;
    } catch (e) {
        deleteFailed = true;
    }
    return `${proxy.a},${setFailed || proxy.a === 1},${deleteFailed || 'a' in proxy}`;
}

function testProxyObservable(): string {
    const changes: string[] = [];
    function observable<T extends object>(obj: T): T {
        return new Proxy(obj, {
            set(target: any, prop: string, value: any) {
                const oldValue = target[prop];
                target[prop] = value;
                changes.push(`${prop}:${oldValue}->${value}`);
                return true;
            }
        });
    }
    const obj = observable({ x: 1, y: 2 });
    obj.x = 10;
    obj.y = 20;
    return changes.join(',');
}

// Warmup
for (let i = 0; i < 20; i++) {
    testProxyGet();
    testProxySet();
    testProxyHas();
    testProxyDeleteProperty();
    testProxyOwnKeys();
    testProxyGetOwnPropertyDescriptor();
    testProxyApply();
    testProxyConstruct();
    testProxyRevocable();
    testProxyValidation();
    testProxyLogging();
    testReflectGet();
    testReflectSet();
    testReflectHas();
    testReflectDeleteProperty();
    testReflectOwnKeys();
    testReflectGetPrototypeOf();
    testReflectSetPrototypeOf();
    testReflectIsExtensible();
    testReflectPreventExtensions();
    testReflectGetOwnPropertyDescriptor();
    testReflectDefineProperty();
    testReflectApply();
    testReflectConstruct();
    testReflectConstructNewTarget();
    testProxyWithReflect();
    testProxyForwarding();
    testProxyNegativeArray();
    testProxyDefaultValues();
    testProxyReadOnly();
    testProxyObservable();
}

// JIT compile
ArkTools.jitCompileAsync(testProxyGet);
ArkTools.jitCompileAsync(testProxySet);
ArkTools.jitCompileAsync(testProxyHas);
ArkTools.jitCompileAsync(testProxyDeleteProperty);
ArkTools.jitCompileAsync(testProxyOwnKeys);
ArkTools.jitCompileAsync(testProxyGetOwnPropertyDescriptor);
ArkTools.jitCompileAsync(testProxyApply);
ArkTools.jitCompileAsync(testProxyConstruct);
ArkTools.jitCompileAsync(testProxyRevocable);
ArkTools.jitCompileAsync(testProxyValidation);
ArkTools.jitCompileAsync(testProxyLogging);
ArkTools.jitCompileAsync(testReflectGet);
ArkTools.jitCompileAsync(testReflectSet);
ArkTools.jitCompileAsync(testReflectHas);
ArkTools.jitCompileAsync(testReflectDeleteProperty);
ArkTools.jitCompileAsync(testReflectOwnKeys);
ArkTools.jitCompileAsync(testReflectGetPrototypeOf);
ArkTools.jitCompileAsync(testReflectSetPrototypeOf);
ArkTools.jitCompileAsync(testReflectIsExtensible);
ArkTools.jitCompileAsync(testReflectPreventExtensions);
ArkTools.jitCompileAsync(testReflectGetOwnPropertyDescriptor);
ArkTools.jitCompileAsync(testReflectDefineProperty);
ArkTools.jitCompileAsync(testReflectApply);
ArkTools.jitCompileAsync(testReflectConstruct);
ArkTools.jitCompileAsync(testReflectConstructNewTarget);
ArkTools.jitCompileAsync(testProxyWithReflect);
ArkTools.jitCompileAsync(testProxyForwarding);
ArkTools.jitCompileAsync(testProxyNegativeArray);
ArkTools.jitCompileAsync(testProxyDefaultValues);
ArkTools.jitCompileAsync(testProxyReadOnly);
ArkTools.jitCompileAsync(testProxyObservable);

print("compile testProxyGet: " + ArkTools.waitJitCompileFinish(testProxyGet));
print("compile testProxySet: " + ArkTools.waitJitCompileFinish(testProxySet));
print("compile testProxyHas: " + ArkTools.waitJitCompileFinish(testProxyHas));
print("compile testProxyDeleteProperty: " + ArkTools.waitJitCompileFinish(testProxyDeleteProperty));
print("compile testProxyOwnKeys: " + ArkTools.waitJitCompileFinish(testProxyOwnKeys));
print("compile testProxyGetOwnPropertyDescriptor: " + ArkTools.waitJitCompileFinish(testProxyGetOwnPropertyDescriptor));
print("compile testProxyApply: " + ArkTools.waitJitCompileFinish(testProxyApply));
print("compile testProxyConstruct: " + ArkTools.waitJitCompileFinish(testProxyConstruct));
print("compile testProxyRevocable: " + ArkTools.waitJitCompileFinish(testProxyRevocable));
print("compile testProxyValidation: " + ArkTools.waitJitCompileFinish(testProxyValidation));
print("compile testProxyLogging: " + ArkTools.waitJitCompileFinish(testProxyLogging));
print("compile testReflectGet: " + ArkTools.waitJitCompileFinish(testReflectGet));
print("compile testReflectSet: " + ArkTools.waitJitCompileFinish(testReflectSet));
print("compile testReflectHas: " + ArkTools.waitJitCompileFinish(testReflectHas));
print("compile testReflectDeleteProperty: " + ArkTools.waitJitCompileFinish(testReflectDeleteProperty));
print("compile testReflectOwnKeys: " + ArkTools.waitJitCompileFinish(testReflectOwnKeys));
print("compile testReflectGetPrototypeOf: " + ArkTools.waitJitCompileFinish(testReflectGetPrototypeOf));
print("compile testReflectSetPrototypeOf: " + ArkTools.waitJitCompileFinish(testReflectSetPrototypeOf));
print("compile testReflectIsExtensible: " + ArkTools.waitJitCompileFinish(testReflectIsExtensible));
print("compile testReflectPreventExtensions: " + ArkTools.waitJitCompileFinish(testReflectPreventExtensions));
print("compile testReflectGetOwnPropertyDescriptor: " + ArkTools.waitJitCompileFinish(testReflectGetOwnPropertyDescriptor));
print("compile testReflectDefineProperty: " + ArkTools.waitJitCompileFinish(testReflectDefineProperty));
print("compile testReflectApply: " + ArkTools.waitJitCompileFinish(testReflectApply));
print("compile testReflectConstruct: " + ArkTools.waitJitCompileFinish(testReflectConstruct));
print("compile testReflectConstructNewTarget: " + ArkTools.waitJitCompileFinish(testReflectConstructNewTarget));
print("compile testProxyWithReflect: " + ArkTools.waitJitCompileFinish(testProxyWithReflect));
print("compile testProxyForwarding: " + ArkTools.waitJitCompileFinish(testProxyForwarding));
print("compile testProxyNegativeArray: " + ArkTools.waitJitCompileFinish(testProxyNegativeArray));
print("compile testProxyDefaultValues: " + ArkTools.waitJitCompileFinish(testProxyDefaultValues));
print("compile testProxyReadOnly: " + ArkTools.waitJitCompileFinish(testProxyReadOnly));
print("compile testProxyObservable: " + ArkTools.waitJitCompileFinish(testProxyObservable));

// Verify
print("testProxyGet: " + testProxyGet());
print("testProxySet: " + testProxySet());
print("testProxyHas: " + testProxyHas());
print("testProxyDeleteProperty: " + testProxyDeleteProperty());
print("testProxyOwnKeys: " + testProxyOwnKeys());
print("testProxyGetOwnPropertyDescriptor: " + testProxyGetOwnPropertyDescriptor());
print("testProxyApply: " + testProxyApply());
print("testProxyConstruct: " + testProxyConstruct());
print("testProxyRevocable: " + testProxyRevocable());
print("testProxyValidation: " + testProxyValidation());
print("testProxyLogging: " + testProxyLogging());
print("testReflectGet: " + testReflectGet());
print("testReflectSet: " + testReflectSet());
print("testReflectHas: " + testReflectHas());
print("testReflectDeleteProperty: " + testReflectDeleteProperty());
print("testReflectOwnKeys: " + testReflectOwnKeys());
print("testReflectGetPrototypeOf: " + testReflectGetPrototypeOf());
print("testReflectSetPrototypeOf: " + testReflectSetPrototypeOf());
print("testReflectIsExtensible: " + testReflectIsExtensible());
print("testReflectPreventExtensions: " + testReflectPreventExtensions());
print("testReflectGetOwnPropertyDescriptor: " + testReflectGetOwnPropertyDescriptor());
print("testReflectDefineProperty: " + testReflectDefineProperty());
print("testReflectApply: " + testReflectApply());
print("testReflectConstruct: " + testReflectConstruct());
print("testReflectConstructNewTarget: " + testReflectConstructNewTarget());
print("testProxyWithReflect: " + testProxyWithReflect());
print("testProxyForwarding: " + testProxyForwarding());
print("testProxyNegativeArray: " + testProxyNegativeArray());
print("testProxyDefaultValues: " + testProxyDefaultValues());
print("testProxyReadOnly: " + testProxyReadOnly());
print("testProxyObservable: " + testProxyObservable());
