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
 * @tc.name:proxy
 * @tc.desc:test Proxy
 * @tc.type: FUNC
 * @tc.require: issue12368
 */
{
    const target = {
        name: 'Alice',
        age: 25,
        city: 'Wonderland'
    };
    const handler = {
        get: function(targetObj, prop, receiver) {
            if (prop === 'age') {
                return targetObj[prop] + 1;
            }
            if (prop === 'name') {
                return 'Mr. ' + targetObj[prop];
            }
            return Reflect.get(targetObj, prop, receiver);
        }
    };
    const proxy = new Proxy(target, handler);
    assert_equal(proxy.name, 'Mr. Alice');
    assert_equal(proxy.age, 26);
    assert_equal(proxy.city, 'Wonderland');
}

/*
 * @tc.name:proxy-get-with-receiver
 */
{
    const target = {
        _value: 10,
        get doubled() {
            return this._value * 2;
        }
    };
    const handler = {
        get: function(targetObj, prop, receiver) {
            if (prop === 'doubled') {
                return Reflect.get(targetObj, prop, receiver) + 5;
            }
            return Reflect.get(targetObj, prop, receiver);
        }
    };
    const proxy = new Proxy(target, handler);
    assert_equal(proxy.doubled, 25);
}

/*
 * @tc.name:proxy-get-inherited
 */
{
    const parent = {
        parentProp: 'from parent'
    };
    const target = Object.create(parent);
    target.ownProp = 'from target';
    const proxy = new Proxy(target, {});
    assert_equal(proxy.ownProp, 'from target');
    assert_equal(proxy.parentProp, 'from parent');
}

/*
 * @tc.name:proxy-get-symbol-key
 */
{
    const sym1 = Symbol('description');
    const sym2 = Symbol('hidden');
    const target = {
        [sym1]: 'symbol value 1',
        [sym2]: 'symbol value 2',
        normal: 'normal value'
    };
    const handler = {
        get: function(targetObj, prop, receiver) {
            const value = Reflect.get(targetObj, prop, receiver);
            if (typeof prop === 'symbol') {
                return 'proxied: ' + value;
            }
            return value;
        }
    };
    const proxy = new Proxy(target, handler);
    assert_equal(proxy[sym1], 'proxied: symbol value 1');
    assert_equal(proxy[sym2], 'proxied: symbol value 2');
    assert_equal(proxy.normal, 'normal value');
}

/*
 * @tc.name:proxy-get-non-existent
 */
{
    const target = {
        existing: 'value'
    };
    const handler = {
        get: function(targetObj, prop, receiver) {
            if (prop in targetObj) {
                return Reflect.get(targetObj, prop, receiver);
            }
            return 'default: ' + prop.toString();
        }
    };
    const proxy = new Proxy(target, handler);
    assert_equal(proxy.existing, 'value');
    assert_equal(proxy.nonExistent, 'default: nonExistent');
}

/*
 * @tc.name:proxy-set-basic
 */
{
    const target = {
        value: 0
    };
    let setValueCount = 0;
    const handler = {
        set: function(targetObj, prop, value, receiver) {
            setValueCount++;
            return Reflect.set(targetObj, prop, value, receiver);
        }
    };
    const proxy = new Proxy(target, handler);
    proxy.value = 10;
    assert_equal(target.value, 10);
    assert_equal(setValueCount, 1);
    proxy.value = 20;
    assert_equal(target.value, 20);
    assert_equal(setValueCount, 2);
}

/*
 * @tc.name:proxy-set-validation
 */
{
    const target = {
        age: 0
    };
    const handler = {
        set: function(targetObj, prop, value, receiver) {
            if (prop === 'age') {
                if (typeof value !== 'number') {
                    return false;
                }
                if (value < 0 || value > 150) {
                    return false;
                }
            }
            return Reflect.set(targetObj, prop, value, receiver);
        }
    };
    const proxy = new Proxy(target, handler);
    proxy.age = 25;
    assert_equal(target.age, 25);
    try {
        proxy.age = -5;
    } catch (e) {
        // Expected: set trap returns false, throws in strict mode
    }
    assert_equal(target.age, 25);
    try {
        proxy.age = 200;
    } catch (e) {
        // Expected: set trap returns false, throws in strict mode
    }
    assert_equal(target.age, 25);
    proxy.age = 50;
    assert_equal(target.age, 50);
}

/*
 * @tc.name:proxy-set-new-property
 */
{
    const target = {};
    const handler = {
        set: function(targetObj, prop, value, receiver) {
            return Reflect.set(targetObj, prop, value.toUpperCase(), receiver);
        }
    };
    const proxy = new Proxy(target, handler);
    proxy.name = 'alice';
    assert_equal(target.name, 'ALICE');
    proxy.city = 'wonderland';
    assert_equal(target.city, 'WONDERLAND');
}

/*
 * @tc.name:proxy-has-basic
 */
{
    const target = {
        public: 'visible',
        _private: 'hidden'
    };
    const handler = {
        has: function(targetObj, prop) {
            if (prop.startsWith('_')) {
                return false;
            }
            return Reflect.has(targetObj, prop);
        }
    };
    const proxy = new Proxy(target, handler);
    assert_equal('public' in proxy, true);
    assert_equal('_private' in proxy, false);
}

/*
 * @tc.name:proxy-has-inherited
 */
{
    const parent = {
        parentProp: 'value'
    };
    const target = Object.create(parent);
    target.ownProp = 'own';
    const handler = {
        has: function(targetObj, prop) {
            return Reflect.has(targetObj, prop);
        }
    };
    const proxy = new Proxy(target, handler);
    assert_equal('ownProp' in proxy, true);
    assert_equal('parentProp' in proxy, true);
    assert_equal('toString' in proxy, true);
}

/*
 * @tc.name:proxy-deleteProperty-basic
 */
{
    const target = {
        a: 1,
        b: 2,
        c: 3
    };
    let deletedProps = [];
    const handler = {
        deleteProperty: function(targetObj, prop) {
            deletedProps.push(prop);
            return Reflect.deleteProperty(targetObj, prop);
        }
    };
    const proxy = new Proxy(target, handler);
    let result = delete proxy.a;
    assert_equal(result, true);
    assert_equal('a' in target, false);
    assert_equal(deletedProps[0], 'a');
    result = delete proxy.b;
    assert_equal(result, true);
    assert_equal('b' in target, false);
}

/*
 * @tc.name:proxy-deleteProperty-prevent
 */
{
    const target = {
        readonly: 'cannot delete',
        normal: 'can delete'
    };
    const handler = {
        deleteProperty: function(targetObj, prop) {
            if (prop === 'readonly') {
                return false;
            }
            return Reflect.deleteProperty(targetObj, prop);
        }
    };
    const proxy = new Proxy(target, handler);
    let errorCaught = false;
    try {
        delete proxy.readonly;
    } catch (e) {
        errorCaught = true;
    }
    assert_equal(errorCaught, true);
    assert_equal('readonly' in proxy, true);
    let result = delete proxy.normal;
    assert_equal(result, true);
    assert_equal('normal' in proxy, false);
}

/*
 * @tc.name:proxy-ownKeys-basic
 */
{
    const target = {
        a: 1,
        b: 2,
        c: 3
    };
    const handler = {
        ownKeys: function(targetObj) {
            const keys = Reflect.ownKeys(targetObj);
            return keys.filter(key => key !== 'b');
        }
    };
    const proxy = new Proxy(target, handler);
    const keys = Reflect.ownKeys(proxy);
    assert_equal(keys.length, 2);
}

/*
 * @tc.name:proxy-ownKeys-with-symbols
 */
{
    const sym1 = Symbol('sym1');
    const sym2 = Symbol('sym2');
    const target = {
        a: 1,
        [sym1]: 'symbol1',
        b: 2,
        [sym2]: 'symbol2'
    };
    const handler = {
        ownKeys: function(targetObj) {
            return Reflect.ownKeys(targetObj);
        }
    };
    const proxy = new Proxy(target, handler);
    const keys = Reflect.ownKeys(proxy);
    assert_equal(keys.length, 4);
}

/*
 * @tc.name:proxy-getOwnPropertyDescriptor-basic
 */
{
    const target = {
        name: 'Alice',
        get fullName() {
            return this.name + ' Smith';
        }
    };
    const handler = {
        getOwnPropertyDescriptor: function(targetObj, prop) {
            const desc = Reflect.getOwnPropertyDescriptor(targetObj, prop);
            if (desc && desc.writable !== undefined) {
                desc.configurable = true;
            }
            return desc;
        }
    };
    const proxy = new Proxy(target, handler);
    const nameDesc = Object.getOwnPropertyDescriptor(proxy, 'name');
    assert_equal(nameDesc.value, 'Alice');
    assert_equal(nameDesc.configurable, true);
    const fullNameDesc = Object.getOwnPropertyDescriptor(proxy, 'fullName');
    assert_equal(typeof fullNameDesc.get, 'function');
}

/*
 * @tc.name:proxy-defineProperty-basic
 */
{
    const target = {};
    let definedProps = [];
    const handler = {
        defineProperty: function(targetObj, prop, descriptor) {
            definedProps.push(prop);
            return Reflect.defineProperty(targetObj, prop, descriptor);
        }
    };
    const proxy = new Proxy(target, handler);
    Object.defineProperty(proxy, 'a', { value: 1, writable: true });
    assert_equal(target.a, 1);
    assert_equal(definedProps[0], 'a');
    Object.defineProperty(proxy, 'b', { value: 2 });
    assert_equal(target.b, 2);
    assert_equal(definedProps[1], 'b');
}

/*
 * @tc.name:proxy-defineProperty-reject
 */
{
    const target = {};
    const handler = {
        defineProperty: function(targetObj, prop, descriptor) {
            if (prop === 'forbidden') {
                return false;
            }
            return Reflect.defineProperty(targetObj, prop, descriptor);
        }
    };
    const proxy = new Proxy(target, handler);
    Object.defineProperty(proxy, 'allowed', { value: 'ok' });
    assert_equal(target.allowed, 'ok');
    let errorCaught = false;
    try {
        Object.defineProperty(proxy, 'forbidden', { value: 'no' });
    } catch (e) {
        errorCaught = true;
    }
    assert_equal(errorCaught, true);
}

/*
 * @tc.name:proxy-construct-basic
 */
{
    function Person(name, age) {
        this.name = name;
        this.age = age;
    }
    Person.prototype.greet = function() {
        return 'Hello, I am ' + this.name;
    };
    const handler = {
        construct: function(target, args, newTarget) {
            return Reflect.construct(target, args, newTarget);
        }
    };
    const ProxyPerson = new Proxy(Person, handler);
    const person = new ProxyPerson('Alice', 25);
    assert_equal(person.name, 'Alice');
    assert_equal(person.age, 25);
    assert_equal(person.greet(), 'Hello, I am Alice');
}

/*
 * @tc.name:proxy-construct-modify
 */
{
    function Rectangle(width, height) {
        this.width = Math.abs(width);
        this.height = Math.abs(height);
    }
    const handler = {
        construct: function(target, args, newTarget) {
            const newArgs = args.map(arg => Math.abs(arg));
            const instance = Reflect.construct(target, newArgs, newTarget);
            instance.area = instance.width * instance.height;
            return instance;
        }
    };
    const ProxyRectangle = new Proxy(Rectangle, handler);
    const rect = new ProxyRectangle(-10, -20);
    assert_equal(rect.width, 10);
    assert_equal(rect.height, 20);
    assert_equal(rect.area, 200);
}

/*
 * @tc.name:proxy-construct-return-object
 */
{
    function MyClass() {
        this.regular = true;
    }
    const handler = {
        construct: function(target, args, newTarget) {
            return { custom: true, proxied: true };
        }
    };
    const ProxyClass = new Proxy(MyClass, handler);
    const instance = new ProxyClass();
    assert_equal(instance.custom, true);
    assert_equal(instance.proxied, true);
    assert_equal(instance.regular, undefined);
}

/*
 * @tc.name:proxy-apply-basic
 */
{
    function sum(a, b, c) {
        return a + b + c;
    }
    const handler = {
        apply: function(target, thisArg, args) {
            return Reflect.apply(target, thisArg, args);
        }
    };
    const proxySum = new Proxy(sum, handler);
    assert_equal(proxySum(1, 2, 3), 6);
    assert_equal(proxySum.call(null, 10, 20, 30), 60);
}

/*
 * @tc.name:proxy-apply-modify-args
 */
{
    function multiply(a, b) {
        return a * b;
    }
    const handler = {
        apply: function(target, thisArg, args) {
            const newArgs = args.map(arg => Math.abs(arg));
            return Reflect.apply(target, thisArg, newArgs) * 2;
        }
    };
    const proxyMultiply = new Proxy(multiply, handler);
    assert_equal(proxyMultiply(-5, 10), 100);
    assert_equal(proxyMultiply(3, 4), 24);
}

/*
 * @tc.name:proxy-apply-with-this
 */
{
    function greet() {
        return 'Hello, ' + this.name;
    }
    const handler = {
        apply: function(target, thisArg, args) {
            const result = Reflect.apply(target, thisArg, args);
            return result + '!';
        }
    };
    const proxyGreet = new Proxy(greet, handler);
    const context = { name: 'Alice' };
    assert_equal(proxyGreet.call(context), 'Hello, Alice!');
    assert_equal(proxyGreet.apply(context), 'Hello, Alice!');
}

/*
 * @tc.name:proxy-getPrototypeOf-basic
 */
{
    const target = {
        name: 'target'
    };
    const proto = {
        shared: 'shared method'
    };
    Object.setPrototypeOf(target, proto);
    const handler = {
        getPrototypeOf: function(targetObj) {
            return Reflect.getPrototypeOf(targetObj);
        }
    };
    const proxy = new Proxy(target, handler);
    const proxyProto = Object.getPrototypeOf(proxy);
    assert_equal(proxyProto.shared, 'shared method');
}

/*
 * @tc.name:proxy-setPrototypeOf-basic
 */
{
    const target = {};
    const newProto = {
        method: function() {
            return 'proto method';
        }
    };
    const handler = {
        setPrototypeOf: function(targetObj, proto) {
            return Reflect.setPrototypeOf(targetObj, proto);
        }
    };
    const proxy = new Proxy(target, handler);
    Object.setPrototypeOf(proxy, newProto);
    assert_equal(Object.getPrototypeOf(proxy), newProto);
    assert_equal(proxy.method(), 'proto method');
}

/*
 * @tc.name:proxy-isExtensible-basic
 */
{
    const target = {
        prop: 'value'
    };
    const handler = {
        isExtensible: function(targetObj) {
            return Reflect.isExtensible(targetObj);
        }
    };
    const proxy = new Proxy(target, handler);
    assert_equal(Reflect.isExtensible(proxy), true);
    Object.preventExtensions(target);
    assert_equal(Reflect.isExtensible(proxy), false);
}

/*
 * @tc.name:proxy-preventExtensions-basic
 */
{
    const target = {
        a: 1,
        b: 2
    };
    const handler = {
        preventExtensions: function(targetObj) {
            return Reflect.preventExtensions(targetObj);
        }
    };
    const proxy = new Proxy(target, handler);
    assert_equal(Reflect.isExtensible(proxy), true);
    Reflect.preventExtensions(proxy);
    assert_equal(Reflect.isExtensible(proxy), false);
    try {
        proxy.c = 3;
    } catch (e) {
        // Expected: cannot add property to non-extensible object in strict mode
    }
    assert_equal('c' in proxy, false);
    assert_equal('c' in target, false);
}

/*
 * @tc.name:proxy-revocable-basic
 */
{
    const target = {
        name: 'Alice',
        age: 25
    };
    const handler = {
        get: function(targetObj, prop, receiver) {
            return Reflect.get(targetObj, prop, receiver);
        },
        set: function(targetObj, prop, value, receiver) {
            return Reflect.set(targetObj, prop, value, receiver);
        }
    };
    const { proxy, revoke } = Proxy.revocable(target, handler);
    assert_equal(proxy.name, 'Alice');
    assert_equal(proxy.age, 25);
    proxy.name = 'Bob';
    assert_equal(target.name, 'Bob');
    revoke();
    let errorCount = 0;
    try {
        const name = proxy.name;
    } catch (e) {
        errorCount++;
    }
    try {
        proxy.age = 30;
    } catch (e) {
        errorCount++;
    }
    assert_equal(errorCount, 2);
}

/*
 * @tc.name:proxy-array-basic
 */
{
    const target = [1, 2, 3, 4, 5];
    const handler = {
        get: function(targetObj, prop, receiver) {
            if (prop === 'length') {
                return targetObj.length;
            }
            const value = Reflect.get(targetObj, prop, receiver);
            return typeof value === 'number' ? value * 2 : value;
        }
    };
    const proxy = new Proxy(target, handler);
    assert_equal(proxy[0], 2);
    assert_equal(proxy[4], 10);
    assert_equal(proxy.length, 5);
}

/*
 * @tc.name:proxy-array-push
 */
{
    const target = [];
    let operations = [];
    const handler = {
        set: function(targetObj, prop, value, receiver) {
            operations.push({ op: 'set', prop: prop, value: value });
            return Reflect.set(targetObj, prop, value, receiver);
        },
        get: function(targetObj, prop, receiver) {
            if (prop === 'push') {
                return function(...args) {
                    operations.push({ op: 'push', args: args });
                    return targetObj.push(...args);
                };
            }
            return Reflect.get(targetObj, prop, receiver);
        }
    };
    const proxy = new Proxy(target, handler);
    proxy.push(1);
    proxy.push(2);
    proxy.push(3);
    assert_equal(target.length, 3);
    assert_equal(target[0], 1);
    assert_equal(target[1], 2);
    assert_equal(target[2], 3);
}

/*
 * @tc.name:proxy-array-iteration
 */
{
    const target = [1, 2, 3, 4, 5];
    const handler = {
        get: function(targetObj, prop, receiver) {
            if (prop === Symbol.iterator) {
                return function* () {
                    for (const item of targetObj) {
                        yield item * 3;
                    }
                };
            }
            return Reflect.get(targetObj, prop, receiver);
        }
    };
    const proxy = new Proxy(target, handler);
    const results = [];
    for (const item of proxy) {
        results.push(item);
    }
    assert_equal(results, [3, 6, 9, 12, 15]);
}

/*
 * @tc.name:proxy-nested
 */
{
    const target = {
        value: 'original'
    };
    const handler1 = {
        get: function(targetObj, prop, receiver) {
            const value = Reflect.get(targetObj, prop, receiver);
            if (prop === 'value') {
                return 'layer1: ' + value;
            }
            return value;
        }
    };
    const handler2 = {
        get: function(targetObj, prop, receiver) {
            const value = Reflect.get(targetObj, prop, receiver);
            if (prop === 'value') {
                return 'layer2: ' + value;
            }
            return value;
        }
    };
    const proxy1 = new Proxy(target, handler1);
    const proxy2 = new Proxy(proxy1, handler2);
    assert_equal(proxy2.value, 'layer2: layer1: original');
    assert_equal(proxy1.value, 'layer1: original');
}

/*
 * @tc.name:proxy-class-instance
 */
{
    class Counter {
        constructor(initial = 0) {
            this._count = initial;
        }
        get count() {
            return this._count;
        }
        increment() {
            this._count++;
        }
        decrement() {
            this._count--;
        }
    }
    const handler = {
        get: function(targetObj, prop, receiver) {
            if (prop === 'count') {
                const value = Reflect.get(targetObj, prop, receiver);
                return value + 100;
            }
            return Reflect.get(targetObj, prop, receiver);
        }
    };
    const counter = new Counter(5);
    const proxyCounter = new Proxy(counter, handler);
    assert_equal(proxyCounter.count, 105);
    counter.increment();
    assert_equal(proxyCounter.count, 106);
}

/*
 * @tc.name:proxy-object-keys
 */
{
    const target = {
        a: 1,
        b: 2,
        _hidden: 3,
        c: 4
    };
    const handler = {
        ownKeys: function(targetObj) {
            return Reflect.ownKeys(targetObj).filter(key => !key.startsWith('_'));
        }
    };
    const proxy = new Proxy(target, handler);
    const keys = Object.keys(proxy);
    assert_equal(keys.length, 3);
}

/*
 * @tc.name:proxy-object-getOwnPropertyNames
 */
{
    const sym = Symbol('symbol');
    const target = {
        a: 1,
        b: 2,
        [sym]: 'symbol value'
    };
    const handler = {
        ownKeys: function(targetObj) {
            return Reflect.ownKeys(targetObj);
        }
    };
    const proxy = new Proxy(target, handler);
    const names = Object.getOwnPropertyNames(proxy);
    assert_equal(names.length, 2);
}

/*
 * @tc.name:proxy-for-in-loop
 */
{
    const target = {
        a: 1,
        b: 2,
        c: 3
    };
    const enumeratedKeys = [];
    const handler = {
        ownKeys: function(targetObj) {
            return Reflect.ownKeys(targetObj);
        },
        getOwnPropertyDescriptor: function(targetObj, prop) {
            const desc = Reflect.getOwnPropertyDescriptor(targetObj, prop);
            if (desc) {
                desc.enumerable = true;
            }
            return desc;
        }
    };
    const proxy = new Proxy(target, handler);
    for (const key in proxy) {
        enumeratedKeys.push(key);
    }
    assert_equal(enumeratedKeys.length >= 3, true);
}

/*
 * @tc.name:proxy-null-handler
 */
{
    const target = {
        name: 'Alice',
        age: 25,
        greet: function() {
            return 'Hello, ' + this.name;
        }
    };
    const proxy = new Proxy(target, {});
    assert_equal(proxy.name, 'Alice');
    assert_equal(proxy.age, 25);
    assert_equal(proxy.greet(), 'Hello, Alice');
    proxy.city = 'Wonderland';
    assert_equal(target.city, 'Wonderland');
}

/*
 * @tc.name:proxy-prototype-chain
 */
{
    const grandParent = {
        grandMethod: function() {
            return 'grand';
        }
    };
    const parent = Object.create(grandParent);
    parent.parentMethod = function() {
        return 'parent';
    };
    const child = Object.create(parent);
    child.childMethod = function() {
        return 'child';
    };
    const proxy = new Proxy(child, {});
    assert_equal(proxy.childMethod(), 'child');
    assert_equal(proxy.parentMethod(), 'parent');
    assert_equal(proxy.grandMethod(), 'grand');
}

/*
 * @tc.name:proxy-toString-tag
 */
{
    const target = {
        [Symbol.toStringTag]: 'ProxiedObject'
    };
    const proxy = new Proxy(target, {});
    const str = Object.prototype.toString.call(proxy);
    assert_equal(str, '[object ProxiedObject]');
}

/*
 * @tc.name:proxy-valueOf
 */
{
    const target = {
        value: 42,
        valueOf: function() {
            return this.value;
        }
    };
    const proxy = new Proxy(target, {});
    assert_equal(proxy.valueOf(), 42);
    assert_equal(Number(proxy), 42);
}

/*
 * @tc.name:proxy-toJSON
 */
{
    const target = {
        name: 'Alice',
        age: 25,
        password: 'secret',
        toJSON: function() {
            return {
                name: this.name,
                age: this.age
            };
        }
    };
    const proxy = new Proxy(target, {});
    const json = JSON.stringify(proxy);
    const parsed = JSON.parse(json);
    assert_equal(parsed.name, 'Alice');
    assert_equal(parsed.age, 25);
    assert_equal('password' in parsed, false);
}

/*
 * @tc.name:proxy-map-proxy
 */
{
    const target = new Map([
        ['key1', 'value1'],
        ['key2', 'value2']
    ]);
    const handler = {
        get: function(targetObj, prop, receiver) {
            const value = Reflect.get(targetObj, prop, receiver);
            if (typeof value === 'function') {
                return value.bind(targetObj);
            }
            return value;
        }
    };
    const proxy = new Proxy(target, handler);
    assert_equal(proxy.get('key1'), 'value1');
    proxy.set('key3', 'value3');
    assert_equal(proxy.get('key3'), 'value3');
    assert_equal(proxy.has('key2'), true);
}

test_end();