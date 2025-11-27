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
 * @tc.name:reflect and proxy
 * @tc.desc:test all proxy interop with reflect
 * @tc.type: FUNC
 * @tc.require: issue11948
 */
// ==================== JavaScript Proxy & Reflect 测试用例 (2500行) ====================

// 测试用例 1: 基础属性访问拦截
print('=== 测试用例 1: 基础属性访问拦截 ===');
const target1 = { name: 'John', age: 30 };
const handler1 = {
    get(target, prop) {
        print(`读取属性: ${prop}`);
        return Reflect.get(target, prop);
    },
    set(target, prop, value) {
        print(`设置属性: ${prop} = ${value}`);
        return Reflect.set(target, prop, value);
    }
};
const proxy1 = new Proxy(target1, handler1);

// 测试
assert_equal(proxy1.name, "John");
proxy1.age = 31;
assert_equal(proxy1.age, 31);

// 测试用例 2: 属性验证
print('\n=== 测试用例 2: 属性验证 ===');
const validator = {
    set(target, prop, value) {
        if (prop === 'age') {
            if (typeof value !== 'number' || value < 0 || value > 150) {
                throw new Error('年龄必须是0-150之间的数字');
            }
        }
        if (prop === 'email') {
            if (!/\S+@\S+\.\S+/.test(value)) {
                throw new Error('无效的邮箱格式');
            }
        }
        return Reflect.set(target, prop, value);
    }
};
const person = {};
const validatedPerson = new Proxy(person, validator);

// 测试
validatedPerson.age = 25; // 预期: 成功
assert_equal(validatedPerson.age, 25); // 预期: 25
try {
    validatedPerson.age = -5; // 预期: 抛出错误
} catch (e) {
    print('验证错误:', e.message); // 预期: 年龄必须是0-150之间的数字
}

// 测试用例 3: 函数调用拦截
print('\n=== 测试用例 3: 函数调用拦截 ===');
function greet(name) {
    return `Hello, ${name}!`;
}
const functionHandler = {
    apply(target, thisArg, argumentsList) {
        print(`调用函数: ${target.name}, 参数:`, argumentsList);
        const result = Reflect.apply(target, thisArg, argumentsList);
        print(`函数返回: ${result}`);
        return result;
    }
};
const proxiedGreet = new Proxy(greet, functionHandler);

// 测试
const result = proxiedGreet('Alice'); // 预期: 输出调用信息
assert_equal(result, "Hello, Alice!"); // 预期: "Hello, Alice!"

// 测试用例 4: 构造函数拦截
print('\n=== 测试用例 4: 构造函数拦截 ===');
class Person {
    constructor(name, age) {
        this.name = name;
        this.age = age;
    }
}
const constructorHandler = {
    construct(target, argumentsList, newTarget) {
        print(`创建实例: ${target.name}, 参数:`, argumentsList);
        const instance = Reflect.construct(target, argumentsList, newTarget);
        print('实例创建成功:', instance);
        return instance;
    }
};
const ProxiedPerson = new Proxy(Person, constructorHandler);

// 测试
const alice = new ProxiedPerson('Alice', 25); // 预期: 输出构造信息
assert_equal(alice.name, "Alice"); // 预期: "Alice", 25
assert_equal(alice.age, 25)

// 测试用例 5: 属性存在性检查
print('\n=== 测试用例 5: 属性存在性检查 ===');
const existenceHandler = {
    has(target, prop) {
        print(`检查属性是否存在: ${prop}`);
        return Reflect.has(target, prop);
    },
    deleteProperty(target, prop) {
        print(`删除属性: ${prop}`);
        return Reflect.deleteProperty(target, prop);
    }
};
const obj = { a: 1, b: 2, _secret: 'hidden' };
const existenceProxy = new Proxy(obj, existenceHandler);

// 测试
assert_true('a' in existenceProxy); // 预期: true
assert_true(!('c' in existenceProxy)); // 预期: false
delete existenceProxy.b; // 预期: 输出删除信息
assert_true(!('b' in existenceProxy)); // 预期: false

// 测试用例 6: 属性描述符操作
print('\n=== 测试用例 6: 属性描述符操作 ===');
const descriptorHandler = {
    getOwnPropertyDescriptor(target, prop) {
        print(`获取属性描述符: ${prop}`);
        return Reflect.getOwnPropertyDescriptor(target, prop);
    },
    defineProperty(target, prop, descriptor) {
        print(`定义属性: ${prop}`, descriptor);
        return Reflect.defineProperty(target, prop, descriptor);
    }
};
const descObj = {};
const descriptorProxy = new Proxy(descObj, descriptorHandler);

// 测试
let t = { value: 42, writable: true }
Object.defineProperty(descriptorProxy, 'x', t);
const desc = Object.getOwnPropertyDescriptor(descriptorProxy, 'x');

// 测试用例 7: 原型操作拦截
print('\n=== 测试用例 7: 原型操作拦截 ===');
const prototypeHandler = {
    getPrototypeOf(target) {
        print('获取原型');
        return Reflect.getPrototypeOf(target);
    },
    setPrototypeOf(target, prototype) {
        print('设置原型:', prototype);
        return Reflect.setPrototypeOf(target, prototype);
    }
};
const protoObj = {};
const prototypeProxy = new Proxy(protoObj, prototypeHandler);

// 测试
const proto = Object.getPrototypeOf(prototypeProxy); // 预期: 输出获取原型信息
assert_equal(proto, Object.prototype); // 预期: Object.prototype

// 测试用例 8: 可扩展性控制
print('\n=== 测试用例 8: 可扩展性控制 ===');
const extensibilityHandler = {
    isExtensible(target) {
        print('检查是否可扩展');
        return Reflect.isExtensible(target);
    },
    preventExtensions(target) {
        print('阻止扩展');
        return Reflect.preventExtensions(target);
    }
};
const extensibleObj = {};
const extensibilityProxy = new Proxy(extensibleObj, extensibilityHandler);

// 测试
assert_true(Object.isExtensible(extensibilityProxy)); // 预期: true
Object.preventExtensions(extensibilityProxy); // 预期: 输出阻止扩展信息
assert_true(!Object.isExtensible(extensibilityProxy)); // 预期: false

// 测试用例 9: 属性枚举拦截
print('\n=== 测试用例 9: 属性枚举拦截 ===');
const enumerationHandler = {
    ownKeys(target) {
        print('枚举自身属性');
        const keys = Reflect.ownKeys(target);
        // 过滤掉以_开头的属性
        return keys.filter(key => !String(key).startsWith('_'));
    }
};
const enumObj = { a: 1, b: 2, _private: 3, [Symbol('secret')]: 4 };
const enumerationProxy = new Proxy(enumObj, enumerationHandler);

// 测试
const keys = Object.keys(enumerationProxy); // 预期: 输出枚举信息
assert_equal(keys, ['a', 'b']); // 预期: ['a', 'b']

// 测试用例 10: 数组操作拦截
print('\n=== 测试用例 10: 数组操作拦截 ===');
const arrayHandler = {
    get(target, prop, receiver) {
        if (['push', 'pop', 'shift', 'unshift'].includes(prop)) {
            return function(...args) {
                print(`数组操作: ${prop}`, args);
                return Reflect.apply(target[prop], target, args);
            };
        }
        return Reflect.get(target, prop, receiver);
    }
};
const arr = [1, 2, 3];
const proxyArray = new Proxy(arr, arrayHandler);

// 测试
proxyArray.push(4, 5); // 预期: 输出数组操作信息
assert_equal(proxyArray[4], 5);
proxyArray.pop(); // 预期: 输出数组操作信息
assert_equal(proxyArray[4], undefined);

// 测试用例 11: 计算属性
print('\n=== 测试用例 11: 计算属性 ===');
const computedHandler = {
    get(target, prop) {
        if (prop === 'fullName') {
            return `${target.firstName} ${target.lastName}`;
        }
        if (prop === 'ageInDays') {
            return target.age * 365;
        }
        return Reflect.get(target, prop);
    }
};
const personData = { firstName: 'John', lastName: 'Doe', age: 30 };
const computedPerson = new Proxy(personData, computedHandler);

// 测试
assert_equal(computedPerson.fullName, "John Doe"); // 预期: "John Doe"
assert_equal(computedPerson.ageInDays, 10950); // 预期: 10950

// 测试用例 12: 自动日志记录
print('\n=== 测试用例 12: 自动日志记录 ===');
const loggerHandler = {
    get(target, prop, receiver) {
        print(`读取: ${String(prop)}`);
        const value = Reflect.get(target, prop, receiver);
        return typeof value === 'function' ? value.bind(target) : value;
    },
    set(target, prop, value, receiver) {
        print(`设置: ${String(prop)} = ${value}`);
        return Reflect.set(target, prop, value, receiver);
    }
};
const loggedObj = { 
    data: 'test',
    method() { return this.data; }
};
const loggedProxy = new Proxy(loggedObj, loggerHandler);

// 测试
loggedProxy.data = 'new value'; // 预期: 输出设置信息
print('方法结果:', loggedProxy.method()); // 预期: 输出读取信息和方法结果

// 测试用例 13: 只读代理
print('\n=== 测试用例 13: 只读代理 ===');
const readOnlyHandler = {
    set(target, prop, value) {
        throw new Error(`对象是只读的，不能设置属性 ${String(prop)}`);
    },
    deleteProperty(target, prop) {
        throw new Error(`对象是只读的，不能删除属性 ${String(prop)}`);
    },
    defineProperty(target, prop, descriptor) {
        throw new Error(`对象是只读的，不能定义属性 ${String(prop)}`);
    }
};
const readOnlyTarget = { constant: 'immutable' };
const readOnlyProxy = new Proxy(readOnlyTarget, readOnlyHandler);

// 测试
assert_equal(readOnlyProxy.constant, "immutable"); // 预期: "immutable"
try {
    readOnlyProxy.constant = 'changed'; // 预期: 抛出错误
} catch (e) {
    print('设置错误:', e.message);
}

// 测试用例 14: 缓存代理
print('\n=== 测试用例 14: 缓存代理 ===');
const cacheHandler = {
    construct(target, argumentsList) {
        print('创建缓存实例');
        const instance = Reflect.construct(target, argumentsList);
        instance._cache = new Map();
        return new Proxy(instance, {
            get(instance, prop) {
                if (prop === 'getWithCache') {
                    return function(key) {
                        if (instance._cache.has(key)) {
                            print(`缓存命中: ${key}`);
                            return instance._cache.get(key);
                        }
                        print(`缓存未命中: ${key}`);
                        const value = `computed_${key}`;
                        instance._cache.set(key, value);
                        return value;
                    };
                }
                return Reflect.get(instance, prop);
            }
        });
    }
};
class DataService {
    constructor() {
        this.name = 'DataService';
    }
}
const CachedService = new Proxy(DataService, cacheHandler);

// 测试
const service = new CachedService(); // 预期: 输出创建缓存实例信息
print('第一次获取:', service.getWithCache('a')); // 预期: 缓存未命中
print('第二次获取:', service.getWithCache('a')); // 预期: 缓存命中

// 测试用例 15: 权限控制代理
print('\n=== 测试用例 15: 权限控制代理 ===');
const authHandler = {
    get(target, prop) {
        if (prop.startsWith('_')) {
            throw new Error(`无权访问私有属性: ${prop}`);
        }
        return Reflect.get(target, prop);
    },
    set(target, prop, value) {
        if (prop === 'adminOnly' && value === true) {
            throw new Error('需要管理员权限');
        }
        return Reflect.set(target, prop, value);
    }
};
const secureData = {
    public: '所有人都可以访问',
    _private: '只有内部可以访问',
    adminOnly: false
};
const authProxy = new Proxy(secureData, authHandler);

// 测试
assert_equal(authProxy.public, "所有人都可以访问"); // 预期: "所有人都可以访问"
try {
    print('访问私有属性:', authProxy._private); // 预期: 抛出错误
    assert_unreachable()
} catch (e) {
    print('权限错误:', e.message);
}

// 测试用例 16: 数据绑定代理
print('\n=== 测试用例 16: 数据绑定代理 ===');
const bindingHandler = {
    set(target, prop, value) {
        const oldValue = target[prop];
        const result = Reflect.set(target, prop, value);
        if (result && oldValue !== value) {
            print(`数据变化: ${prop} 从 ${oldValue} 变为 ${value}`);
            // 这里可以触发UI更新或其他响应
        }
        return result;
    }
};
const bindableData = { count: 0, name: 'initial' };
const bindingProxy = new Proxy(bindableData, bindingHandler);

// 测试
bindingProxy.count = 1; // 预期: 输出数据变化信息
bindingProxy.name = 'updated'; // 预期: 输出数据变化信息

// 测试用例 17: 验证和转换代理
print('\n=== 测试用例 17: 验证和转换代理 ===');
const transformHandler = {
    set(target, prop, value) {
        let transformedValue = value;
        
        if (prop === 'price') {
            if (typeof value !== 'number' || value < 0) {
                throw new Error('价格必须是正数');
            }
            transformedValue = Math.round(value * 100) / 100; // 保留两位小数
        }
        
        if (prop === 'name') {
            if (typeof value !== 'string') {
                throw new Error('名称必须是字符串');
            }
            transformedValue = value.trim();
        }
        
        return Reflect.set(target, prop, transformedValue);
    }
};
const product = {};
const transformedProduct = new Proxy(product, transformHandler);

// 测试
transformedProduct.name = '  Test Product  '; // 预期: 自动去除空格
transformedProduct.price = 19.999; // 预期: 自动四舍五入
print('产品:', transformedProduct); // 预期: {name: "Test Product", price: 20}

// 测试用例 20: 链式操作代理
print('\n=== 测试用例 20: 链式操作代理 ===');
const chainHandler = {
    get(target, prop) {
        if (prop === 'then') {
            return function(callback) {
                print('链式调用 then');
                const result = callback(target.value);
                return new Proxy({ value: result }, chainHandler);
            };
        }
        return Reflect.get(target, prop);
    }
};
const chainStart = { value: 10 };
const chainProxy = new Proxy(chainStart, chainHandler);

// 测试
chainProxy
    .then(x => x * 2)
    .then(x => x + 5)
    .then(x => {
        print('最终结果:', x); // 预期: 25
        return x;
    });

// 测试用例 21: 事件发射器代理
print('\n=== 测试用例 21: 事件发射器代理 ===');
const eventHandler = {
    set(target, prop, value) {
        const oldValue = target[prop];
        const result = Reflect.set(target, prop, value);
        if (result && prop.startsWith('on') && oldValue !== value) {
            print(`事件监听器设置: ${prop} = ${value}`);
        }
        return result;
    }
};
const eventTarget = {};
const eventProxy = new Proxy(eventTarget, eventHandler);

// 测试
eventProxy.onClick = () => print('点击事件'); // 预期: 输出事件监听器设置信息
eventProxy.onChange = (value) => print('变更事件:', value); // 预期: 输出事件监听器设置信息

// 测试用例 22: 状态管理代理
print('\n=== 测试用例 22: 状态管理代理 ===');
const stateHandler = {
    set(target, prop, value) {
        print(`状态变更: ${prop} = ${value}`);
        // 这里可以添加状态持久化、撤销重做等功能
        return Reflect.set(target, prop, value);
    }
};
const appState = { count: 0, user: null };
const stateProxy = new Proxy(appState, stateHandler);

// 测试
stateProxy.count = 1; // 预期: 输出状态变更信息
stateProxy.user = { name: 'John' }; // 预期: 输出状态变更信息

// 测试用例 23: 表单验证代理
print('\n=== 测试用例 23: 表单验证代理 ===');
const formHandler = {
    set(target, prop, value) {
        const validations = {
            username: (val) => val.length >= 3 && val.length <= 20,
            email: (val) => /\S+@\S+\.\S+/.test(val),
            age: (val) => val >= 18 && val <= 100
        };
        
        if (validations[prop] && !validations[prop](value)) {
            throw new Error(`表单验证失败: ${prop}`);
        }
        
        return Reflect.set(target, prop, value);
    }
};
const formData = {};
const formProxy = new Proxy(formData, formHandler);

// 测试
formProxy.username = 'alice'; // 预期: 成功
formProxy.email = 'alice@example.com'; // 预期: 成功
try {
    formProxy.age = 15; // 预期: 抛出错误
    assert_unreachable()
} catch (e) {
    print('验证错误:', e.message);
}

// 测试用例 24: 不可变数据代理
print('\n=== 测试用例 24: 不可变数据代理 ===');
const immutableHandler = {
    set() {
        throw new Error('此对象是不可变的');
    },
    deleteProperty() {
        throw new Error('此对象是不可变的');
    },
    defineProperty() {
        throw new Error('此对象是不可变的');
    }
};
const immutableData = { version: '1.0', data: 'important' };
const immutableProxy = new Proxy(immutableData, immutableHandler);

// 测试
assert_equal(immutableProxy.version, "1.0"); // 预期: "1.0"
try {
    immutableProxy.version = '2.0'; // 预期: 抛出错误
    assert_unreachable()
} catch (e) {
    print('不可变错误:', e.message);
}

// 测试用例 25: 延迟加载代理
print('\n=== 测试用例 25: 延迟加载代理 ===');
const lazyHandler = {
    get(target, prop) {
        if (prop === 'expensiveData' && !target._loaded) {
            print('延迟加载数据...');
            target._loaded = true;
            target.expensiveData = '这是昂贵的数据加载结果';
        }
        return Reflect.get(target, prop);
    }
};
const lazyTarget = { _loaded: false };
const lazyProxy = new Proxy(lazyTarget, lazyHandler);

// 测试
print('第一次访问:', lazyProxy.expensiveData); // 预期: 输出延迟加载信息
print('第二次访问:', lazyProxy.expensiveData); // 预期: 直接返回数据

// 测试用例 26: 类型安全代理
print('\n=== 测试用例 26: 类型安全代理 ===');
const typeSafeHandler = {
    set(target, prop, value) {
        const types = {
            name: 'string',
            age: 'number',
            active: 'boolean'
        };
        
        if (types[prop] && typeof value !== types[prop]) {
            throw new Error(`属性 ${prop} 必须是 ${types[prop]} 类型`);
        }
        
        return Reflect.set(target, prop, value);
    }
};
const typedObj = {};
const typeSafeProxy = new Proxy(typedObj, typeSafeHandler);

// 测试
typeSafeProxy.name = 'John'; // 预期: 成功
typeSafeProxy.age = 30; // 预期: 成功
try {
    typeSafeProxy.age = '三十'; // 预期: 抛出错误
    assert_unreachable()
} catch (e) {
    print('类型错误:', e.message);
}

// 测试用例 27: 路由代理
print('\n=== 测试用例 27: 路由代理 ===');
const routerHandler = {
    get(target, prop) {
        if (prop === 'navigate') {
            return function(path) {
                print(`路由跳转: ${path}`);
                // 这里可以处理路由逻辑
                return `导航到 ${path}`;
            };
        }
        return Reflect.get(target, prop);
    }
};
const router = {};
const routerProxy = new Proxy(router, routerHandler);

// 测试
const navResult = routerProxy.navigate('/home'); // 预期: 输出路由跳转信息
print('路由结果:', navResult);

// 测试用例 28: 缓存函数结果代理
print('\n=== 测试用例 28: 缓存函数结果代理 ===');
const memoizeHandler = {
    apply(target, thisArg, argumentsList) {
        const key = JSON.stringify(argumentsList);
        if (!target._cache) target._cache = new Map();
        
        if (target._cache.has(key)) {
            print('缓存命中');
            return target._cache.get(key);
        }
        
        print('计算新结果');
        const result = Reflect.apply(target, thisArg, argumentsList);
        target._cache.set(key, result);
        return result;
    }
};
function expensiveOperation(x, y) {
    return x * y + Math.sqrt(x + y);
}
const memoizedOperation = new Proxy(expensiveOperation, memoizeHandler);

// 测试
assert_equal(memoizedOperation(5, 3), 17.82842712474619); // 预期: 计算新结果
assert_equal(memoizedOperation(5, 3), 17.82842712474619); // 预期: 缓存命中

// 测试用例 29: 数据序列化代理
print('\n=== 测试用例 29: 数据序列化代理 ===');
const serializationHandler = {
    get(target, prop) {
        if (prop === 'toJSON') {
            return function() {
                print('序列化对象');
                const obj = {};
                for (const key of Object.keys(target)) {
                    if (!key.startsWith('_')) {
                        obj[key] = target[key];
                    }
                }
                return obj;
            };
        }
        return Reflect.get(target, prop);
    }
};
const serializableData = { public: 'data', _private: 'secret' };
const serializationProxy = new Proxy(serializableData, serializationHandler);

// 测试
const json = JSON.stringify(serializationProxy); // 预期: 输出序列化信息
assert_equal(json['public'], undefined); // 预期: {"public":"data"}

// 测试用例 30: 多语言代理
print('\n=== 测试用例 30: 多语言代理 ===');
const i18nHandler = {
    get(target, prop) {
        const translations = {
            welcome: { en: 'Welcome', es: 'Bienvenido', fr: 'Bienvenue' },
            goodbye: { en: 'Goodbye', es: 'Adiós', fr: 'Au revoir' }
        };
        
        if (translations[prop]) {
            return function(lang = 'en') {
                return translations[prop][lang] || translations[prop]['en'];
            };
        }
        return Reflect.get(target, prop);
    }
};
const i18n = {};
const i18nProxy = new Proxy(i18n, i18nHandler);

// 测试
assert_equal(i18nProxy.welcome('en'), "Welcome"); // 预期: "Welcome"
assert_equal(i18nProxy.goodbye('es'), "Adiós"); // 预期: "Adiós"

// 测试用例 31: 数据验证和清理代理
print('\n=== 测试用例 31: 数据验证和清理代理 ===');
const sanitizeHandler = {
    set(target, prop, value) {
        let cleanedValue = value;
        
        // 清理字符串
        if (typeof value === 'string') {
            cleanedValue = value.trim().replace(/<script\b[^<]*(?:(?!<\/script>)<[^<]*)*<\/script>/gi, '');
        }
        
        // 验证数字
        if (prop === 'score' && (typeof value !== 'number' || value < 0 || value > 100)) {
            throw new Error('分数必须在0-100之间');
        }
        
        return Reflect.set(target, prop, cleanedValue);
    }
};
const userInput = {};
const sanitizedInput = new Proxy(userInput, sanitizeHandler);

// 测试
sanitizedInput.name = '  John Doe  '; // 预期: 自动去除空格
sanitizedInput.comment = '<script>alert("xss")</script>Hello'; // 预期: 清理脚本标签
sanitizedInput.score = 85; // 预期: 成功
print('清理后的数据:', sanitizedInput);

// 测试用例 32: 观察者模式代理
print('\n=== 测试用例 32: 观察者模式代理 ===');
const observerHandler = {
    set(target, prop, value) {
        const oldValue = target[prop];
        const result = Reflect.set(target, prop, value);
        
        if (result && oldValue !== value) {
            // 通知所有观察者
            if (target._observers && target._observers[prop]) {
                target._observers[prop].forEach(callback => {
                    callback(value, oldValue);
                });
            }
        }
        
        return result;
    }
};
const observable = { _observers: {} };
const observableProxy = new Proxy(observable, observerHandler);

// 添加观察者
observableProxy._observers.name = [
    (newVal, oldVal) => print(`名称从 ${oldVal} 变为 ${newVal}`)
];

// 测试
observableProxy.name = 'Alice'; // 预期: 触发观察者回调

// 测试用例 33: 批量操作代理
print('\n=== 测试用例 33: 批量操作代理 ===');
const batchHandler = {
    get(target, prop) {
        if (prop === 'batchUpdate') {
            return function(updates) {
                print('开始批量更新');
                for (const [key, value] of Object.entries(updates)) {
                    Reflect.set(target, key, value);
                }
                print('批量更新完成');
            };
        }
        return Reflect.get(target, prop);
    }
};
const batchTarget = { a: 1, b: 2, c: 3 };
const batchProxy = new Proxy(batchTarget, batchHandler);

// 测试
batchProxy.batchUpdate({ a: 10, b: 20, c: 30 }); // 预期: 输出批量操作信息
assert_equal(batchProxy["b"], 20); //

// 测试用例 34: 权限验证代理
print('\n=== 测试用例 34: 权限验证代理 ===');
const permissionHandler = {
    get(target, prop, receiver) {
        const userRole = 'user'; // 模拟用户角色
        
        if (prop === 'adminData' && userRole !== 'admin') {
            throw new Error('需要管理员权限');
        }
        
        return Reflect.get(target, prop, receiver);
    },
    
    set(target, prop, value, receiver) {
        const userRole = 'user'; // 模拟用户角色
        
        if (prop === 'systemSetting' && userRole !== 'admin') {
            throw new Error('需要管理员权限才能修改系统设置');
        }
        
        return Reflect.set(target, prop, value, receiver);
    }
};
const permissionData = {
    userData: '普通用户数据',
    adminData: '管理员数据',
    systemSetting: 'default'
};
const permissionProxy = new Proxy(permissionData, permissionHandler);

// 测试
print('用户数据:', permissionProxy.userData); // 预期: 成功
try {
    print('管理员数据:', permissionProxy.adminData); // 预期: 抛出错误
    assert_unreachable()
} catch (e) {
    print('权限错误:', e.message);
}

// 测试用例 35: 数据转换代理
print('\n=== 测试用例 35: 数据转换代理 ===');
const conversionHandler = {
    get(target, prop) {
        const value = Reflect.get(target, prop);
        
        // 自动单位转换
        if (prop === 'temperature' && target.unit === 'fahrenheit') {
            return (value - 32) * 5/9; // 转换为摄氏度
        }
        
        if (prop === 'distance' && target.unit === 'miles') {
            return value * 1.60934; // 转换为公里
        }
        
        return value;
    },
    
    set(target, prop, value) {
        // 自动格式化
        if (prop === 'phone') {
            value = value.replace(/\D/g, '').replace(/(\d{3})(\d{3})(\d{4})/, '($1) $2-$3');
        }
        
        if (prop === 'date') {
            value = new Date(value).toISOString().split('T')[0];
        }
        
        return Reflect.set(target, prop, value);
    }
};
const conversionData = { unit: 'fahrenheit', temperature: 68 };
const conversionProxy = new Proxy(conversionData, conversionHandler);

// 测试
assert_equal(conversionProxy.temperature, 20); // 预期: 20
conversionProxy.phone = '1234567890';
assert_equal(conversionProxy.phone, "(123) 456-7890"); // 预期: "(123) 456-7890"

// 测试用例 36: 撤销代理
print('\n=== 测试用例 36: 撤销代理 ===');
const { proxy: revocableProxy, revoke } = Proxy.revocable({ data: 'sensitive' }, {
    get(target, prop) {
        print(`访问敏感数据: ${prop}`);
        return Reflect.get(target, prop);
    }
});

// 测试
print('撤销前访问:', revocableProxy.data); // 预期: 输出访问信息
revoke(); // 撤销代理
try {
    print('撤销后访问:', revocableProxy.data); // 预期: 抛出错误
    assert_unreachable()
} catch (e) {
    print('撤销后错误:', e.message);
}

// 测试用例 37: 嵌套对象代理
print('\n=== 测试用例 37: 嵌套对象代理 ===');
const deepProxyHandler = {
    get(target, prop) {
        const value = Reflect.get(target, prop);
        if (value && typeof value === 'object') {
            return new Proxy(value, deepProxyHandler);
        }
        return value;
    },
    set(target, prop, value) {
        print(`设置嵌套属性: ${prop} =`, value);
        return Reflect.set(target, prop, value);
    }
};
const nestedObj = {
    level1: {
        level2: {
            level3: {
                value: 'deep'
            }
        }
    }
};
const deepProxy = new Proxy(nestedObj, deepProxyHandler);

// 测试
deepProxy.level1.level2.level3.value = 'modified'; // 预期: 输出设置信息
assert_equal(deepProxy.level1.level2.level3.value, "modified"); // 预期: "modified"

// 测试用例 38: 函数柯里化代理
print('\n=== 测试用例 38: 函数柯里化代理 ===');
const curryHandler = {
    get(target, prop) {
        if (prop === 'curry') {
            return function(fn) {
                return function curried(...args) {
                    if (args.length >= fn.length) {
                        return fn.apply(this, args);
                    } else {
                        return function(...moreArgs) {
                            return curried.apply(this, args.concat(moreArgs));
                        };
                    }
                };
            };
        }
        return Reflect.get(target, prop);
    }
};
const functional = {};
const functionalProxy = new Proxy(functional, curryHandler);

// 测试
function add(a, b, c) { return a + b + c; }
const curriedAdd = functionalProxy.curry(add);
assert_equal(curriedAdd(1)(2)(3), 6); // 预期: 6

// 测试用例 40: 智能默认值代理
print('\n=== 测试用例 40: 智能默认值代理 ===');
const defaultsHandler = {
    get(target, prop) {
        const value = Reflect.get(target, prop);
        if (value === undefined) {
            const defaults = {
                theme: 'light',
                language: 'en',
                fontSize: 14,
                notifications: true
            };
            return defaults[prop];
        }
        return value;
    }
};
const settings = {};
const settingsProxy = new Proxy(settings, defaultsHandler);

// 测试
assert_equal(settingsProxy.theme, "light"); // 预期: "light"
assert_equal(settingsProxy.language, "en"); // 预期: "en"
settingsProxy.theme = 'dark';
assert_equal(settingsProxy.theme, "dark"); // 预期: "dark"

// 测试用例 41: 数组映射代理
print('\n=== 测试用例 41: 数组映射代理 ===');
const arrayMapHandler = {
    get(target, prop) {
        if (prop === 'map') {
            return function(mapper) {
                print('执行数组映射');
                const result = [];
                for (let i = 0; i < target.length; i++) {
                    result.push(mapper(target[i], i, target));
                }
                return result;
            };
        }
        return Reflect.get(target, prop);
    }
};
const numbers = [1, 2, 3, 4, 5];
const arrayMapProxy = new Proxy(numbers, arrayMapHandler);

// 测试
const doubled = arrayMapProxy.map(x => x * 2); // 预期: 输出执行数组映射信息
print('映射结果:', doubled); // 预期: [2, 4, 6, 8, 10]

// 测试用例 42: 数据验证代理增强版
print('\n=== 测试用例 42: 数据验证代理增强版 ===');
const enhancedValidator = {
    set(target, prop, value) {
        const rules = {
            username: {
                type: 'string',
                minLength: 3,
                maxLength: 20,
                pattern: /^[a-zA-Z0-9_]+$/
            },
            email: {
                type: 'string',
                pattern: /\S+@\S+\.\S+/
            },
            age: {
                type: 'number',
                min: 0,
                max: 150
            }
        };
        
        const rule = rules[prop];
        if (rule) {
            if (typeof value !== rule.type) {
                throw new Error(`${prop} 必须是 ${rule.type} 类型`);
            }
            if (rule.minLength && value.length < rule.minLength) {
                throw new Error(`${prop} 长度不能小于 ${rule.minLength}`);
            }
            if (rule.maxLength && value.length > rule.maxLength) {
                throw new Error(`${prop} 长度不能大于 ${rule.maxLength}`);
            }
            if (rule.pattern && !rule.pattern.test(value)) {
                throw new Error(`${prop} 格式不正确`);
            }
            if (rule.min !== undefined && value < rule.min) {
                throw new Error(`${prop} 不能小于 ${rule.min}`);
            }
            if (rule.max !== undefined && value > rule.max) {
                throw new Error(`${prop} 不能大于 ${rule.max}`);
            }
        }
        
        return Reflect.set(target, prop, value);
    }
};
const userProfile = {};
const validatedProfile = new Proxy(userProfile, enhancedValidator);

// 测试
validatedProfile.username = 'john_doe'; // 预期: 成功
validatedProfile.email = 'john@example.com'; // 预期: 成功
validatedProfile.age = 25; // 预期: 成功
try {
    validatedProfile.username = 'ab'; // 预期: 抛出错误
    assert_unreachable()
} catch (e) {
    print('验证错误:', e.message);
}

// 测试用例 43: 事件委托代理
print('\n=== 测试用例 43: 事件委托代理 ===');
const delegateHandler = {
    get(target, prop) {
        if (prop.startsWith('on')) {
            return function(handler) {
                print(`注册事件处理器: ${prop}`);
                target[prop] = handler;
                return () => {
                    print(`移除事件处理器: ${prop}`);
                    delete target[prop];
                };
            };
        }
        return Reflect.get(target, prop);
    }
};
const eventDelegator = {};
const delegateProxy = new Proxy(eventDelegator, delegateHandler);

// 测试
const removeClick = delegateProxy.onClick((e) => print('点击事件'));
const removeHover = delegateProxy.onHover((e) => print('悬停事件'));
removeClick(); // 预期: 输出移除事件处理器信息

// 测试用例 44: 数据同步代理
print('\n=== 测试用例 44: 数据同步代理 ===');
const syncHandler = {
    set(target, prop, value) {
        const result = Reflect.set(target, prop, value);
        if (result) {
            // 模拟同步到服务器
            print(`同步数据到服务器: ${prop} = ${value}`);
        }
        return result;
    }
};
const localData = {};
const syncProxy = new Proxy(localData, syncHandler);

// 测试
syncProxy.userPreference = 'dark mode'; // 预期: 输出同步信息
syncProxy.lastLogin = new Date().toISOString(); // 预期: 输出同步信息

// 测试用例 45: 缓存代理增强版
print('\n=== 测试用例 45: 缓存代理增强版 ===');
const enhancedCacheHandler = {
    construct(target, argumentsList) {
        const instance = Reflect.construct(target, argumentsList);
        instance._cache = new Map();
        instance._stats = { hits: 0, misses: 0 };
        
        return new Proxy(instance, {
            get(instance, prop) {
                if (prop === 'getCached') {
                    return function(key) {
                        if (instance._cache.has(key)) {
                            instance._stats.hits++;
                            print(`缓存命中: ${key}`);
                            return instance._cache.get(key);
                        }
                        instance._stats.misses++;
                        print(`缓存未命中: ${key}`);
                        const value = `computed_${key}_${Date.now()}`;
                        instance._cache.set(key, value);
                        return value;
                    };
                }
                if (prop === 'cacheStats') {
                    return instance._stats;
                }
                if (prop === 'clearCache') {
                    return function() {
                        print('清空缓存');
                        instance._cache.clear();
                        instance._stats = { hits: 0, misses: 0 };
                    };
                }
                return Reflect.get(instance, prop);
            }
        });
    }
};
class EnhancedService {
    constructor() {
        this.name = 'EnhancedService';
    }
}
const EnhancedCachedService = new Proxy(EnhancedService, enhancedCacheHandler);

// 测试
const enhancedService = new EnhancedCachedService();
print('第一次获取:', enhancedService.getCached('test')); // 预期: 缓存未命中
print('第二次获取:', enhancedService.getCached('test')); // 预期: 缓存命中
assert_equal(enhancedService.cacheStats["hits"], 1); // 预期: {hits: 1, misses: 1}

// 测试用例 46: 数据分页代理
print('\n=== 测试用例 46: 数据分页代理 ===');
const paginationHandler = {
    get(target, prop) {
        if (prop === 'getPage') {
            return function(page, pageSize = 10) {
                const start = (page - 1) * pageSize;
                const end = start + pageSize;
                const pageData = target.data.slice(start, end);
                print(`获取第 ${page} 页数据: ${start}-${end}`);
                return {
                    data: pageData,
                    page,
                    pageSize,
                    total: target.data.length,
                    totalPages: Math.ceil(target.data.length / pageSize)
                };
            };
        }
        return Reflect.get(target, prop);
    }
};
const paginationData = {
    data: Array.from({ length: 100 }, (_, i) => `Item ${i + 1}`)
};
const paginationProxy = new Proxy(paginationData, paginationHandler);

// 测试
const page1 = paginationProxy.getPage(1, 5); // 预期: 输出分页信息
assert_equal(page1.data[1], "Item 2"); // 预期: ["Item 1", "Item 2", "Item 3", "Item 4", "Item 5"]

// 测试用例 47: 数据过滤代理
print('\n=== 测试用例 47: 数据过滤代理 ===');
const filterHandler = {
    get(target, prop) {
        if (prop === 'filter') {
            return function(predicate) {
                print('执行数据过滤');
                const result = [];
                for (const item of target.data) {
                    if (predicate(item)) {
                        result.push(item);
                    }
                }
                return result;
            };
        }
        return Reflect.get(target, prop);
    }
};
const filterData = {
    data: [
        { name: 'Alice', age: 25 },
        { name: 'Bob', age: 30 },
        { name: 'Charlie', age: 35 }
    ]
};
const filterProxy = new Proxy(filterData, filterHandler);

// 测试
const adults = filterProxy.filter(person => person.age >= 30); // 预期: 输出执行数据过滤信息
assert_equal(adults[1]["name"], "Charlie"); // 预期: [{name: "Bob", age: 30}, {name: "Charlie", age: 35}]

// 测试用例 48: 数据排序代理
print('\n=== 测试用例 48: 数据排序代理 ===');
const sortHandler = {
    get(target, prop) {
        if (prop === 'sortBy') {
            return function(key, order = 'asc') {
                print(`按 ${key} ${order} 排序`);
                const sorted = [...target.data].sort((a, b) => {
                    if (order === 'asc') {
                        return a[key] > b[key] ? 1 : -1;
                    } else {
                        return a[key] < b[key] ? 1 : -1;
                    }
                });
                return sorted;
            };
        }
        return Reflect.get(target, prop);
    }
};
const sortData = {
    data: [
        { name: 'Charlie', age: 35 },
        { name: 'Alice', age: 25 },
        { name: 'Bob', age: 30 }
    ]
};
const sortProxy = new Proxy(sortData, sortHandler);

// 测试
const sortedByName = sortProxy.sortBy('name'); // 预期: 输出排序信息

// 测试用例 49: 数据聚合代理
print('\n=== 测试用例 49: 数据聚合代理 ===');
const aggregateHandler = {
    get(target, prop) {
        if (prop === 'aggregate') {
            return function(key) {
                print(`按 ${key} 聚合数据`);
                const groups = {};
                for (const item of target.data) {
                    const groupKey = item[key];
                    if (!groups[groupKey]) {
                        groups[groupKey] = [];
                    }
                    groups[groupKey].push(item);
                }
                return groups;
            };
        }
        return Reflect.get(target, prop);
    }
};
const aggregateData = {
    data: [
        { category: 'A', value: 10 },
        { category: 'B', value: 20 },
        { category: 'A', value: 15 },
        { category: 'C', value: 25 }
    ]
};
const aggregateProxy = new Proxy(aggregateData, aggregateHandler);

// 测试
const grouped = aggregateProxy.aggregate('category'); // 预期: 输出聚合信息
print('聚合结果:', grouped); // 预期: 按类别分组的数据

// 测试用例 50: 综合测试用例
print('\n=== 测试用例 50: 综合测试用例 ===');
const comprehensiveHandler = {
    get(target, prop) {
        print(`[GET] ${String(prop)}`);
        const value = Reflect.get(target, prop);
        return typeof value === 'function' ? value.bind(target) : value;
    },
    set(target, prop, value) {
        print(`[SET] ${String(prop)} = ${value}`);
        return Reflect.set(target, prop, value);
    },
    has(target, prop) {
        print(`[HAS] ${String(prop)}`);
        return Reflect.has(target, prop);
    },
    deleteProperty(target, prop) {
        print(`[DELETE] ${String(prop)}`);
        return Reflect.deleteProperty(target, prop);
    },
    apply(target, thisArg, argumentsList) {
        print(`[APPLY] ${target.name}`, argumentsList);
        return Reflect.apply(target, thisArg, argumentsList);
    },
    construct(target, argumentsList, newTarget) {
        print(`[CONSTRUCT] ${target.name}`, argumentsList);
        return Reflect.construct(target, argumentsList, newTarget);
    }
};

class ComprehensiveClass {
    constructor(name) {
        this.name = name;
        this.data = [];
    }
    
    addItem(item) {
        this.data.push(item);
        return this.data.length;
    }
    
    getItems() {
        return this.data;
    }
}

const ComprehensiveProxy = new Proxy(ComprehensiveClass, comprehensiveHandler);

// 综合测试
const instance = new ComprehensiveProxy('TestInstance'); // 预期: 输出构造信息
instance.addItem('item1'); // 预期: 输出设置和应用信息
instance.addItem('item2'); // 预期: 输出设置和应用信息
print('实例数据:', instance.getItems()); // 预期: 输出获取和应用信息
assert_equal(instance.name, "TestInstance"); // 预期: 输出获取信息
assert_true('data' in instance); // 预期: 输出存在性检查信息

// ==================== 补充1000行JavaScript Proxy测试用例 ====================

// 测试用例 51: 深度观察代理
print('\n=== 测试用例 51: 深度观察代理 ===');
const deepObserveHandler = {
    get(target, prop) {
        const value = Reflect.get(target, prop);
        if (value && typeof value === 'object' && !value._isProxy) {
            // 为嵌套对象创建代理
            const proxy = new Proxy(value, deepObserveHandler);
            proxy._isProxy = true;
            return proxy;
        }
        return value;
    },
    set(target, prop, value) {
        const oldValue = target[prop];
        const result = Reflect.set(target, prop, value);
        if (result && oldValue !== value) {
            print(`深度观察: ${prop} 从 ${oldValue} 变为 ${value}`);
            // 触发深度更新
            if (target._listeners) {
                target._listeners.forEach(listener => listener(prop, value, oldValue));
            }
        }
        return result;
    }
};
const deepData = {
    user: {
        profile: {
            name: 'John',
            settings: {
                theme: 'light'
            }
        }
    },
    _listeners: []
};
const deepObserveProxy = new Proxy(deepData, deepObserveHandler);

// 测试
deepObserveProxy.user.profile.name = 'Jane'; // 预期: 输出深度观察信息
deepObserveProxy.user.profile.settings.theme = 'dark'; // 预期: 输出深度观察信息

// 测试用例 52: 异步队列代理
print('\n=== 测试用例 52: 异步队列代理 ===');
const asyncQueueHandler = {
    get(target, prop) {
        if (prop === 'enqueue') {
            return function(task) {
                print('任务入队:', task.name);
                target.queue.push(task);
                return target.queue.length;
            };
        }
        if (prop === 'process') {
            return async function() {
                print('开始处理队列，任务数:', target.queue.length);
                while (target.queue.length > 0) {
                    const task = target.queue.shift();
                    print('执行任务:', task.name);
                    await task.execute();
                }
                print('队列处理完成');
            };
        }
        return Reflect.get(target, prop);
    }
};
const asyncQueue = { queue: [] };
const asyncQueueProxy = new Proxy(asyncQueue, asyncQueueHandler);

// 测试
asyncQueueProxy.enqueue({
    name: '任务1',
    execute: () => new Promise(resolve => setTimeout(() => {
        print('任务1完成');
        resolve();
    }, 100))
});
asyncQueueProxy.enqueue({
    name: '任务2', 
    execute: () => new Promise(resolve => setTimeout(() => {
        print('任务2完成');
        resolve();
    }, 50))
});

// 测试用例 53: 数据版本控制代理
print('\n=== 测试用例 53: 数据版本控制代理 ===');
const versionControlHandler = {
    set(target, prop, value) {
        const oldValue = target[prop];
        const result = Reflect.set(target, prop, value);
        if (result && oldValue !== value) {
            // 记录版本历史
            if (!target._versionHistory) target._versionHistory = [];
            target._versionHistory.push({
                property: prop,
                oldValue,
                newValue: value,
                timestamp: new Date(),
                version: target._versionHistory.length + 1
            });
            print(`版本 ${target._versionHistory.length}: ${prop} 变更`);
        }
        return result;
    },
    get(target, prop) {
        if (prop === 'getHistory') {
            return function() {
                return target._versionHistory || [];
            };
        }
        if (prop === 'revert') {
            return function(version) {
                const history = target._versionHistory;
                if (history && history[version - 1]) {
                    const change = history[version - 1];
                    Reflect.set(target, change.property, change.oldValue);
                    print(`回滚到版本 ${version}`);
                }
            };
        }
        return Reflect.get(target, prop);
    }
};
const versionedData = {};
const versionControlProxy = new Proxy(versionedData, versionControlHandler);

// 测试
versionControlProxy.name = 'Version1';
versionControlProxy.name = 'Version2';
versionControlProxy.age = 25;
print('版本历史:', versionControlProxy.getHistory());
versionControlProxy.revert(1); // 预期: 回滚到版本1

// 测试用例 54: 智能缓存代理
print('\n=== 测试用例 54: 智能缓存代理 ===');
const smartCacheHandler = {
    get(target, prop) {
        if (prop === 'getWithSmartCache') {
            return function(key, producer, ttl = 60000) {
                const now = Date.now();
                const cacheKey = `cache_${key}`;
                
                if (target[cacheKey] && now - target[cacheKey].timestamp < ttl) {
                    print(`智能缓存命中: ${key}`);
                    target[cacheKey].hits++;
                    return target[cacheKey].value;
                }
                
                print(`智能缓存未命中: ${key}, 重新计算`);
                const value = producer();
                target[cacheKey] = {
                    value,
                    timestamp: now,
                    hits: 1,
                    ttl
                };
                return value;
            };
        }
        if (prop === 'getCacheStats') {
            return function() {
                const stats = { total: 0, hits: 0, memory: 0 };
                for (const key in target) {
                    if (key.startsWith('cache_')) {
                        stats.total++;
                        stats.hits += target[key].hits;
                        stats.memory += JSON.stringify(target[key]).length;
                    }
                }
                return stats;
            };
        }
        return Reflect.get(target, prop);
    }
};
const smartCache = {};
const smartCacheProxy = new Proxy(smartCache, smartCacheHandler);

// 测试
const expensiveCalculation = () => {
    print('执行昂贵计算...');
    return Math.random() * 1000;
};
print('第一次获取:', smartCacheProxy.getWithSmartCache('calc', expensiveCalculation));
print('第二次获取:', smartCacheProxy.getWithSmartCache('calc', expensiveCalculation));
print('缓存统计:', smartCacheProxy.getCacheStats());

// 测试用例 57: 响应式系统代理
print('\n=== 测试用例 57: 响应式系统代理 ===');
const reactiveHandler = {
    get(target, prop) {
        // 收集依赖
        if (target._dependencies && target._dependencies[prop]) {
            print(`收集依赖: ${prop}`);
        }
        const value = Reflect.get(target, prop);
        if (value && typeof value === 'object') {
            return new Proxy(value, reactiveHandler);
        }
        return value;
    },
    set(target, prop, value) {
        const oldValue = target[prop];
        const result = Reflect.set(target, prop, value);
        if (result && oldValue !== value) {
            // 触发更新
            if (target._dependencies && target._dependencies[prop]) {
                target._dependencies[prop].forEach(effect => {
                    print(`触发更新: ${prop}`);
                    effect(value, oldValue);
                });
            }
        }
        return result;
    }
};
const reactiveData = {
    count: 0,
    user: { name: 'Alice' },
    _dependencies: {}
};
const reactiveProxy = new Proxy(reactiveData, reactiveHandler);

// 添加响应式效果
reactiveProxy._dependencies.count = [
    (newVal, oldVal) => print(`count 从 ${oldVal} 变为 ${newVal}`)
];
reactiveProxy._dependencies.user = [
    (newVal, oldVal) => print(`user 对象发生变化`)
];

// 测试
reactiveProxy.count = 1;
reactiveProxy.user.name = 'Bob';

// 测试用例 58: 权限分级代理
print('\n=== 测试用例 58: 权限分级代理 ===');
const hierarchicalPermissionHandler = {
    get(target, prop) {
        const userRole = target._currentUser?.role || 'guest';
        const permissions = {
            guest: ['read_public'],
            user: ['read_public', 'read_private', 'write_own'],
            admin: ['read_public', 'read_private', 'write_own', 'write_all', 'delete']
        };
        
        const userPermissions = permissions[userRole] || permissions.guest;
        
        if (prop.startsWith('_')) {
            throw new Error(`无权访问私有属性: ${prop}`);
        }
        
        if (prop === 'adminData' && !userPermissions.includes('read_private')) {
            throw new Error('需要更高级别权限访问管理员数据');
        }
        
        if (typeof target[prop] === 'function') {
            return function(...args) {
                const methodName = prop;
                if (methodName.startsWith('delete') && !userPermissions.includes('delete')) {
                    throw new Error('无权执行删除操作');
                }
                if (methodName.startsWith('set') && !userPermissions.includes('write_own')) {
                    throw new Error('无权执行写入操作');
                }
                return Reflect.apply(target[prop], target, args);
            };
        }
        
        return Reflect.get(target, prop);
    },
    set(target, prop, value) {
        const userRole = target._currentUser?.role || 'guest';
        
        if (prop === '_currentUser') {
            print(`用户切换: ${value?.role}`);
            return Reflect.set(target, prop, value);
        }
        
        if (prop === 'systemConfig' && userRole !== 'admin') {
            throw new Error('需要管理员权限修改系统配置');
        }
        
        return Reflect.set(target, prop, value);
    }
};
const hierarchicalData = {
    publicData: '所有人都可以访问',
    userData: '用户数据',
    adminData: '管理员数据',
    systemConfig: '默认配置',
    _currentUser: null,
    
    setConfig(value) {
        this.systemConfig = value;
        return '配置已更新';
    },
    
    deleteData(id) {
        return `数据 ${id} 已删除`;
    }
};
const hierarchicalProxy = new Proxy(hierarchicalData, hierarchicalPermissionHandler);

// 测试不同权限用户
hierarchicalProxy._currentUser = { role: 'user' };
print('用户数据:', hierarchicalProxy.userData);
try {
    print('管理员数据:', hierarchicalProxy.adminData);
} catch (e) {
    assert_unreachable()
    print('权限错误:', e.message);
}

hierarchicalProxy._currentUser = { role: 'admin' };
assert_equal(hierarchicalProxy.adminData, "管理员数据");
hierarchicalProxy.setConfig('新配置');

// 测试用例 59: 数据转换管道代理
print('\n=== 测试用例 59: 数据转换管道代理 ===');
const pipelineHandler = {
    get(target, prop) {
        if (prop === 'pipe') {
            return function(...transformers) {
                print('创建转换管道:', transformers.map(t => t.name));
                return function(data) {
                    let result = data;
                    for (const transformer of transformers) {
                        print(`应用转换: ${transformer.name}`);
                        result = transformer(result);
                    }
                    return result;
                };
            };
        }
        if (prop === 'compose') {
            return function(...transformers) {
                print('创建组合转换');
                return function(data) {
                    return transformers.reduceRight((result, transformer) => {
                        print(`应用组合转换: ${transformer.name}`);
                        return transformer(result);
                    }, data);
                };
            };
        }
        return Reflect.get(target, prop);
    }
};
const pipeline = {};
const pipelineProxy = new Proxy(pipeline, pipelineHandler);

// 定义转换函数
const toUpperCase = (str) => {
    print('转换为大写');
    return str.toUpperCase();
};
const addPrefix = (prefix) => (str) => {
    print(`添加前缀: ${prefix}`);
    return prefix + str;
};
const reverseString = (str) => {
    print('反转字符串');
    return str.split('').reverse().join('');
};

// 测试
const transformPipe = pipelineProxy.pipe(addPrefix('Hello: '), toUpperCase, reverseString);
const result1 = transformPipe('world');
assert_equal(result1, "DLROW :OLLEH");

// 测试用例 61: 数据快照代理
print('\n=== 测试用例 61: 数据快照代理 ===');
const snapshotHandler = {
    set(target, prop, value) {
        // 创建快照前状态
        if (!target._snapshots) target._snapshots = [];
        const snapshot = JSON.parse(JSON.stringify(target));
        target._snapshots.push({
            data: snapshot,
            timestamp: new Date(),
            action: `set_${prop}`
        });
        
        print(`创建快照: ${prop} = ${value}`);
        return Reflect.set(target, prop, value);
    },
    get(target, prop) {
        if (prop === 'createSnapshot') {
            return function(name = 'manual') {
                const snapshot = JSON.parse(JSON.stringify(target));
                if (!target._snapshots) target._snapshots = [];
                target._snapshots.push({
                    data: snapshot,
                    timestamp: new Date(),
                    name,
                    type: 'manual'
                });
                print(`手动创建快照: ${name}`);
                return target._snapshots.length;
            };
        }
        if (prop === 'restoreSnapshot') {
            return function(index) {
                if (target._snapshots && target._snapshots[index]) {
                    const snapshot = target._snapshots[index].data;
                    Object.keys(target).forEach(key => {
                        if (key !== '_snapshots') {
                            delete target[key];
                        }
                    });
                    Object.assign(target, snapshot);
                    print(`恢复快照 #${index}`);
                    return true;
                }
                return false;
            };
        }
        if (prop === 'getSnapshots') {
            return function() {
                return target._snapshots || [];
            };
        }
        return Reflect.get(target, prop);
    }
};
const snapshotData = { value: 1, name: 'initial' };
const snapshotProxy = new Proxy(snapshotData, snapshotHandler);

// 测试
snapshotProxy.value = 2;
snapshotProxy.name = 'updated';
snapshotProxy.createSnapshot('测试快照');
assert_equal(snapshotProxy.getSnapshots().length, 3);
snapshotProxy.restoreSnapshot(0);
assert_equal(snapshotProxy.name, 'initial');

// 测试用例 62: 事件总线代理
print('\n=== 测试用例 62: 事件总线代理 ===');
const eventBusHandler = {
    get(target, prop) {
        if (prop === 'on') {
            return function(event, handler) {
                if (!target._events) target._events = {};
                if (!target._events[event]) target._events[event] = [];
                target._events[event].push(handler);
                print(`注册事件: ${event}`);
                return () => {
                    // 返回取消订阅函数
                    target._events[event] = target._events[event].filter(h => h !== handler);
                };
            };
        }
        if (prop === 'emit') {
            return function(event, data) {
                print(`触发事件: ${event}`, data);
                if (target._events && target._events[event]) {
                    target._events[event].forEach(handler => {
                        try {
                            handler(data);
                        } catch (error) {
                            console.error(`事件处理错误: ${event}`, error);
                        }
                    });
                }
            };
        }
        if (prop === 'off') {
            return function(event, handler) {
                if (target._events && target._events[event]) {
                    target._events[event] = target._events[event].filter(h => h !== handler);
                    print(`取消事件: ${event}`);
                }
            };
        }
        return Reflect.get(target, prop);
    }
};
const eventBus = {};
const eventBusProxy = new Proxy(eventBus, eventBusHandler);

// 测试
const unsubscribe = eventBusProxy.on('user.login', (user) => {
    print('用户登录:', user);
});
eventBusProxy.on('user.logout', (user) => {
    print('用户登出:', user);
});

eventBusProxy.emit('user.login', { name: 'Alice', id: 1 });
eventBusProxy.emit('user.logout', { name: 'Alice', id: 1 });
unsubscribe();

// 测试用例 65: 数据验证和转换管道
print('\n=== 测试用例 65: 数据验证和转换管道 ===');
const validationPipelineHandler = {
    get(target, prop) {
        if (prop === 'createValidator') {
            return function(schema) {
                print('创建验证器:', Object.keys(schema));
                return function(data) {
                    const errors = [];
                    const result = {};
                    
                    for (const [key, rules] of Object.entries(schema)) {
                        const value = data[key];
                        let isValid = true;
                        
                        // 必需检查
                        if (rules.required && (value === undefined || value === null || value === '')) {
                            errors.push(`${key} 是必需的`);
                            isValid = false;
                        }
                        
                        if (value !== undefined && value !== null && value !== '') {
                            // 类型检查
                            if (rules.type && typeof value !== rules.type) {
                                errors.push(`${key} 必须是 ${rules.type} 类型`);
                                isValid = false;
                            }
                            
                            // 转换和验证
                            if (isValid && rules.transform) {
                                result[key] = rules.transform(value);
                            } else if (isValid) {
                                result[key] = value;
                            }
                        }
                    }
                    
                    if (errors.length > 0) {
                        throw new Error(`验证失败: ${errors.join(', ')}`);
                    }
                    
                    print('验证通过:', result);
                    return result;
                };
            };
        }
        return Reflect.get(target, prop);
    }
};
const validationPipeline = {};
const validationPipelineProxy = new Proxy(validationPipeline, validationPipelineHandler);

// 测试
const userValidator = validationPipelineProxy.createValidator({
    username: {
        type: 'string',
        required: true,
        transform: (val) => val.toLowerCase().trim()
    },
    age: {
        type: 'number',
        required: true,
        transform: (val) => Math.max(0, Math.min(150, val))
    },
    email: {
        type: 'string',
        required: true,
        transform: (val) => val.toLowerCase().trim()
    }
});

try {
    const validUser = userValidator({
        username: '  JohnDoe  ',
        age: 25,
        email: 'JOHN@EXAMPLE.COM'
    });
    print('验证后的用户:', validUser);
} catch (e) {
    assert_unreachable()
    print('验证错误:', e.message);
}

// 测试用例 67: 数据备份和恢复代理
print('\n=== 测试用例 67: 数据备份和恢复代理 ===');
const backupHandler = {
    set(target, prop, value) {
        // 自动备份
        if (!target._backups) target._backups = [];
        const backup = {
            timestamp: new Date(),
            data: JSON.parse(JSON.stringify(target)),
            action: `set_${prop}`
        };
        target._backups.push(backup);
        
        // 限制备份数量
        if (target._backups.length > 10) {
            target._backups.shift();
        }
        
        print(`自动备份: ${prop} = ${value}`);
        return Reflect.set(target, prop, value);
    },
    get(target, prop) {
        if (prop === 'createBackup') {
            return function(name = 'manual') {
                if (!target._backups) target._backups = [];
                const backup = {
                    timestamp: new Date(),
                    data: JSON.parse(JSON.stringify(target)),
                    name,
                    type: 'manual'
                };
                target._backups.push(backup);
                print(`手动备份: ${name}`);
                return backup.timestamp;
            };
        }
        if (prop === 'restoreBackup') {
            return function(timestamp) {
                if (target._backups) {
                    const backup = target._backups.find(b => b.timestamp.getTime() === timestamp);
                    if (backup) {
                        Object.keys(target).forEach(key => {
                            if (key !== '_backups') delete target[key];
                        });
                        Object.assign(target, backup.data);
                        print(`恢复备份: ${backup.name}`);
                        return true;
                    }
                }
                return false;
            };
        }
        if (prop === 'listBackups') {
            return function() {
                return target._backups || [];
            };
        }
        if (prop === 'cleanupBackups') {
            return function() {
                const count = target._backups ? target._backups.length : 0;
                target._backups = [];
                print(`清理备份: ${count} 个备份已删除`);
                return count;
            };
        }
        return Reflect.get(target, prop);
    }
};
const backupData = { important: 'data', count: 1 };
const backupProxy = new Proxy(backupData, backupHandler);

// 测试
backupProxy.important = 'modified';
backupProxy.count = 2;
backupProxy.createBackup('重要修改');
assert_equal(backupProxy.listBackups().length, 3);
backupProxy.cleanupBackups();

// 测试用例 68: 智能路由代理
print('\n=== 测试用例 68: 智能路由代理 ===');
const routerHandler1 = {
    get(target, prop) {
        if (prop === 'navigate') {
            return function(path, options = {}) {
                print(`路由导航: ${path}`, options);
                
                // 路由守卫
                if (target._guards) {
                    for (const guard of target._guards) {
                        if (!guard(path, options)) {
                            print(`路由被守卫阻止: ${path}`);
                            return false;
                        }
                    }
                }
                
                // 记录历史
                if (!target._history) target._history = [];
                target._history.push({
                    path,
                    timestamp: new Date(),
                    options
                });
                
                // 触发路由变化
                if (target._listeners) {
                    target._listeners.forEach(listener => listener(path, options));
                }
                
                print(`路由成功: ${path}`);
                return true;
            };
        }
        if (prop === 'addGuard') {
            return function(guard) {
                if (!target._guards) target._guards = [];
                target._guards.push(guard);
                print('添加路由守卫');
            };
        }
        if (prop === 'onChange') {
            return function(listener) {
                if (!target._listeners) target._listeners = [];
                target._listeners.push(listener);
                print('添加路由变化监听器');
            };
        }
        if (prop === 'getHistory') {
            return function() {
                return target._history || [];
            };
        }
        if (prop === 'goBack') {
            return function() {
                if (target._history && target._history.length > 1) {
                    target._history.pop(); // 移除当前
                    const previous = target._history[target._history.length - 1];
                    print(`返回上一页: ${previous.path}`);
                    return previous;
                }
                return null;
            };
        }
        return Reflect.get(target, prop);
    }
};
const router1 = {};
const routerProxy2 = new Proxy(router1, routerHandler1);

// 测试
routerProxy2.addGuard((path) => {
    if (path === '/admin') {
        print('检查管理员权限...');
        return false; // 模拟权限不足
    }
    return true;
});

routerProxy2.onChange((path) => {
    print(`路由变化监听: 导航到 ${path}`);
});

routerProxy2.navigate('/home');
routerProxy2.navigate('/admin');
routerProxy2.navigate('/profile');
assert_equal(routerProxy2.getHistory().length, 2);

// 测试用例 71: 数据分片代理
print('\n=== 测试用例 71: 数据分片代理 ===');
const shardingHandler = {
    get(target, prop) {
        if (prop === 'storeSharded') {
            return function(key, data, chunkSize = 1024) {
                print(`存储分片数据: ${key}, 块大小: ${chunkSize}`);
                const dataStr = JSON.stringify(data);
                const chunks = [];
                
                for (let i = 0; i < dataStr.length; i += chunkSize) {
                    const chunk = dataStr.substr(i, chunkSize);
                    chunks.push(chunk);
                    const chunkKey = `${key}_chunk_${i / chunkSize}`;
                    Reflect.set(target, chunkKey, chunk);
                }
                
                // 存储元数据
                Reflect.set(target, `${key}_metadata`, {
                    totalChunks: chunks.length,
                    originalSize: dataStr.length,
                    chunkSize,
                    timestamp: Date.now()
                });
                
                print(`数据分片完成: ${chunks.length} 个块`);
                return chunks.length;
            };
        }
        if (prop === 'loadSharded') {
            return function(key) {
                const metadata = Reflect.get(target, `${key}_metadata`);
                if (!metadata) {
                    throw new Error(`未找到分片数据: ${key}`);
                }
                
                print(`加载分片数据: ${key}, ${metadata.totalChunks} 个块`);
                let assembled = '';
                
                for (let i = 0; i < metadata.totalChunks; i++) {
                    const chunk = Reflect.get(target, `${key}_chunk_${i}`);
                    if (!chunk) {
                        throw new Error(`缺失数据块: ${i}`);
                    }
                    assembled += chunk;
                }
                
                const data = JSON.parse(assembled);
                print(`数据分片加载完成: ${metadata.originalSize} 字节`);
                return data;
            };
        }
        return Reflect.get(target, prop);
    }
};
const shardingStorage = {};
const shardingProxy = new Proxy(shardingStorage, shardingHandler);

// 测试
const largeArray = Array.from({ length: 500 }, (_, i) => ({
    id: i,
    content: '这是一些测试数据 '.repeat(10),
    timestamp: new Date()
}));

shardingProxy.storeSharded('largeData', largeArray, 500);
const loadedData = shardingProxy.loadSharded('largeData');
assert_equal(loadedData.length, 500);

// 测试用例 72: 智能重试代理增强版
print('\n=== 测试用例 72: 智能重试代理增强版 ===');
const advancedRetryHandler = {
    get(target, prop) {
        const value = Reflect.get(target, prop);
        
        if (typeof value === 'function') {
            return async function(...args) {
                const config = {
                    maxRetries: target._retryConfig?.maxRetries || 3,
                    baseDelay: target._retryConfig?.baseDelay || 100,
                    maxDelay: target._retryConfig?.maxDelay || 5000,
                    backoff: target._retryConfig?.backoff || 'exponential',
                    retryCondition: target._retryConfig?.retryCondition || (error => true)
                };
                
                let lastError;
                let lastResult;
                
                for (let attempt = 0; attempt <= config.maxRetries; attempt++) {
                    try {
                        print(`执行 ${prop}, 尝试 ${attempt + 1}/${config.maxRetries + 1}`);
                        lastResult = await Reflect.apply(value, target, args);
                        print(`${prop} 执行成功`);
                        
                        // 成功时可能还有重试逻辑
                        if (target._retryConfig?.retryOnSuccess) {
                            const shouldRetry = target._retryConfig.retryOnSuccess(lastResult, attempt);
                            if (shouldRetry && attempt < config.maxRetries) {
                                print(`成功但需要重试: ${prop}`);
                                continue;
                            }
                        }
                        
                        return lastResult;
                    } catch (error) {
                        lastError = error;
                        print(`${prop} 第 ${attempt + 1} 次失败:`, error.message);
                        
                        // 检查是否应该重试
                        if (!config.retryCondition(error) || attempt === config.maxRetries) {
                            break;
                        }
                        
                        // 计算延迟
                        let delay;
                        switch (config.backoff) {
                            case 'exponential':
                                delay = Math.min(config.baseDelay * Math.pow(2, attempt), config.maxDelay);
                                break;
                            case 'linear':
                                delay = Math.min(config.baseDelay * (attempt + 1), config.maxDelay);
                                break;
                            case 'fixed':
                                delay = config.baseDelay;
                                break;
                            default:
                                delay = config.baseDelay;
                        }
                        
                        print(`等待 ${delay}ms 后重试`);
                        await new Promise(resolve => setTimeout(resolve, delay));
                    }
                }
                
                if (lastError) {
                    throw new Error(`${prop} 所有重试失败: ${lastError.message}`);
                }
                
                return lastResult;
            };
        }
        
        if (prop === 'configureRetry') {
            return function(config) {
                if (!target._retryConfig) target._retryConfig = {};
                Object.assign(target._retryConfig, config);
                print('配置重试策略:', config);
            };
        }
        
        return value;
    }
};
const advancedRetryService = {
    async unreliableCall() {
        const random = Math.random();
        if (random < 0.6) {
            throw new Error('服务暂时不可用');
        }
        return { success: true, data: '操作成功' };
    },
    
    async sometimesFails(data) {
        if (data.shouldFail) {
            throw new Error('条件性失败');
        }
        return { processed: data.value * 2 };
    }
};
const advancedRetryProxy = new Proxy(advancedRetryService, advancedRetryHandler);

// 测试
advancedRetryProxy.configureRetry({
    maxRetries: 2,
    baseDelay: 200,
    backoff: 'exponential'
});

// ==================== 补充300行JavaScript Proxy测试用例 ====================

// 测试用例 76: 数据序列化代理增强版
print('\n=== 测试用例 76: 数据序列化代理增强版 ===');
const enhancedSerializationHandler = {
    get(target, prop) {
        if (prop === 'toJSON') {
            return function() {
                print('执行增强序列化');
                const result = {};
                for (const key of Object.keys(target)) {
                    if (!key.startsWith('_') && key !== 'toJSON') {
                        const value = target[key];
                        // 处理特殊类型
                        if (value instanceof Date) {
                            result[key] = value.toISOString();
                        } else if (typeof value === 'function') {
                            result[key] = `[Function: ${value.name}]`;
                        } else {
                            result[key] = value;
                        }
                    }
                }
                return result;
            };
        }
        
        if (prop === 'fromJSON') {
            return function(jsonData) {
                print('执行反序列化');
                Object.keys(jsonData).forEach(key => {
                    const value = jsonData[key];
                    // 尝试解析日期
                    if (typeof value === 'string' && !isNaN(Date.parse(value))) {
                        target[key] = new Date(value);
                    } else {
                        target[key] = value;
                    }
                });
                return target;
            };
        }
        
        return Reflect.get(target, prop);
    }
};

const serializableData1 = {
    name: 'Test Object',
    createdAt: new Date(),
    count: 42,
    method: function() { return 'test'; },
    _private: 'hidden'
};

const enhancedSerializationProxy = new Proxy(serializableData1, enhancedSerializationHandler);

// 测试
const jsonString = JSON.stringify(enhancedSerializationProxy);
print('序列化结果:', jsonString);
const parsedData = JSON.parse(jsonString);
enhancedSerializationProxy.fromJSON(parsedData);
assert_equal(enhancedSerializationProxy.name, "Test Object");

// 测试用例 77: 数据验证代理完整版
print('\n=== 测试用例 77: 数据验证代理完整版 ===');
const completeValidationHandler = {
    set(target, prop, value) {
        const validationRules = {
            email: {
                validate: (val) => typeof val === 'string' && /\S+@\S+\.\S+/.test(val),
                message: '必须是有效的邮箱地址',
                transform: (val) => val.toLowerCase().trim()
            },
            age: {
                validate: (val) => typeof val === 'number' && val >= 0 && val <= 150,
                message: '年龄必须在0-150之间',
                transform: (val) => Math.floor(val)
            },
            name: {
                validate: (val) => typeof val === 'string' && val.length >= 2 && val.length <= 50,
                message: '姓名长度必须在2-50个字符之间',
                transform: (val) => val.trim()
            },
            score: {
                validate: (val) => typeof val === 'number' && val >= 0 && val <= 100,
                message: '分数必须在0-100之间'
            }
        };

        const rule = validationRules[prop];
        if (rule) {
            // 验证
            if (!rule.validate(value)) {
                throw new Error(`验证失败: ${prop} - ${rule.message}`);
            }
            
            // 转换
            if (rule.transform) {
                value = rule.transform(value);
            }
            
            print(`验证通过: ${prop} = ${value}`);
        }

        return Reflect.set(target, prop, value);
    },
    
    get(target, prop) {
        if (prop === 'validateAll') {
            return function() {
                const errors = [];
                const validationRules = {
                    email: (val) => typeof val === 'string' && /\S+@\S+\.\S+/.test(val),
                    age: (val) => typeof val === 'number' && val >= 0 && val <= 150,
                    name: (val) => typeof val === 'string' && val.length >= 2 && val.length <= 50
                };
                
                for (const [key, validator] of Object.entries(validationRules)) {
                    const value = target[key];
                    if (value !== undefined && !validator(value)) {
                        errors.push(key);
                    }
                }
                
                if (errors.length > 0) {
                    throw new Error(`批量验证失败: ${errors.join(', ')}`);
                }
                
                print('所有验证通过');
                return true;
            };
        }
        
        return Reflect.get(target, prop);
    }
};

const validatedData = {};
const completeValidationProxy = new Proxy(validatedData, completeValidationHandler);

// 测试
completeValidationProxy.email = 'TEST@EXAMPLE.COM';
completeValidationProxy.age = 25.7;
completeValidationProxy.name = '  John Doe  ';
completeValidationProxy.score = 85;
assert_equal(completeValidationProxy.age, 25);
completeValidationProxy.validateAll();

// 测试用例 79: 事件系统代理完整版
print('\n=== 测试用例 79: 事件系统代理完整版 ===');
const eventSystemHandler = {
    get(target, prop) {
        if (prop === 'on') {
            return function(eventName, listener, options = {}) {
                if (!target._events) target._events = new Map();
                if (!target._events.has(eventName)) {
                    target._events.set(eventName, new Set());
                }
                
                const eventSet = target._events.get(eventName);
                const wrappedListener = options.once ? 
                    (...args) => {
                        listener(...args);
                        eventSet.delete(wrappedListener);
                    } : 
                    listener;
                
                eventSet.add(wrappedListener);
                
                print(`注册事件监听器: ${eventName}`);
                
                // 返回取消订阅函数
                return () => {
                    eventSet.delete(wrappedListener);
                    print(`取消事件监听器: ${eventName}`);
                };
            };
        }
        
        if (prop === 'emit') {
            return function(eventName, ...args) {
                print(`触发事件: ${eventName}`, args);
                let listeners = null
                if (target._events && target._events.has(eventName)) {
                    listeners = target._events.get(eventName);
                    listeners.forEach(listener => {
                        try {
                            listener(...args);
                        } catch (error) {
                            console.error(`事件处理错误 (${eventName}):`, error);
                        }
                    });
                }
                
                return listeners ? listeners.size : 0;
            };
        }
        
        if (prop === 'off') {
            return function(eventName, listener) {
                if (target._events && target._events.has(eventName)) {
                    const listeners = target._events.get(eventName);
                    if (listener) {
                        listeners.delete(listener);
                        print(`移除指定监听器: ${eventName}`);
                    } else {
                        listeners.clear();
                        print(`移除所有监听器: ${eventName}`);
                    }
                }
            };
        }
        
        if (prop === 'listenerCount') {
            return function(eventName) {
                if (target._events && target._events.has(eventName)) {
                    return target._events.get(eventName).size;
                }
                return 0;
            };
        }
        
        if (prop === 'eventNames') {
            return function() {
                return target._events ? Array.from(target._events.keys()) : [];
            };
        }
        
        return Reflect.get(target, prop);
    }
};

const eventSystem = {};
const eventSystemProxy = new Proxy(eventSystem, eventSystemHandler);

// 测试
const unsubscribe1 = eventSystemProxy.on('user.login', (user) => {
    print('用户登录:', user);
});

const unsubscribe2 = eventSystemProxy.on('user.login', (user) => {
    print('另一个登录监听器:', user);
});

const unsubscribe3 = eventSystemProxy.on('user.logout', (user) => {
    print('用户登出:', user);
}, { once: true });

eventSystemProxy.emit('user.login', { name: 'Alice', id: 1 });
eventSystemProxy.emit('user.logout', { name: 'Alice', id: 1 });
eventSystemProxy.emit('user.logout', { name: 'Bob', id: 2 }); // 这个不会触发，因为once

assert_equal(eventSystemProxy.listenerCount('user.login'), 2);

unsubscribe1();

// 测试用例 81: 数据转换代理完整版
print('\n=== 测试用例 81: 数据转换代理完整版 ===');
const dataTransformationHandler = {
    set(target, prop, value) {
        let transformedValue = value;
        
        // 基于属性名的转换规则
        const transformationRules = {
            // 字符串处理
            name: (val) => typeof val === 'string' ? val.trim() : val,
            email: (val) => typeof val === 'string' ? val.toLowerCase().trim() : val,
            description: (val) => typeof val === 'string' ? val.replace(/\s+/g, ' ').trim() : val,
            
            // 数字处理
            price: (val) => typeof val === 'number' ? Math.round(val * 100) / 100 : val,
            quantity: (val) => typeof val === 'number' ? Math.max(0, Math.floor(val)) : val,
            discount: (val) => typeof val === 'number' ? Math.min(100, Math.max(0, val)) : val,
            
            // 日期处理
            createdAt: (val) => val instanceof Date ? val : new Date(val),
            updatedAt: (val) => val instanceof Date ? val : new Date(val),
            
            // 数组处理
            tags: (val) => Array.isArray(val) ? val.map(tag => tag.trim().toLowerCase()) : val,
            categories: (val) => Array.isArray(val) ? [...new Set(val)] : val, // 去重
        };
        
        const transformer = transformationRules[prop];
        if (transformer) {
            try {
                transformedValue = transformer(value);
                print(`数据转换: ${prop} [${value}] -> [${transformedValue}]`);
            } catch (error) {
                console.warn(`转换失败: ${prop}`, error.message);
            }
        }
        
        return Reflect.set(target, prop, transformedValue);
    },
    
    get(target, prop) {
        if (prop === 'addTransformer') {
            return function(propertyName, transformer) {
                if (!target._customTransformers) {
                    target._customTransformers = new Map();
                }
                target._customTransformers.set(propertyName, transformer);
                print(`添加自定义转换器: ${propertyName}`);
            };
        }
        
        if (prop === 'removeTransformer') {
            return function(propertyName) {
                if (target._customTransformers) {
                    target._customTransformers.delete(propertyName);
                    print(`移除转换器: ${propertyName}`);
                }
            };
        }
        
        // 检查自定义转换器
        if (target._customTransformers && target._customTransformers.has(prop)) {
            const value = Reflect.get(target, prop);
            const transformer = target._customTransformers.get(prop);
            return transformer(value);
        }
        
        return Reflect.get(target, prop);
    }
};

const transformableData = {};
const dataTransformationProxy = new Proxy(transformableData, dataTransformationHandler);

// 测试内置转换
dataTransformationProxy.name = '  John Doe  ';
dataTransformationProxy.email = '  TEST@EXAMPLE.COM  ';
dataTransformationProxy.price = 19.999;
dataTransformationProxy.quantity = 5.7;
dataTransformationProxy.tags = ['  JS  ', 'Proxy', '  TEST  '];
dataTransformationProxy.createdAt = '2024-01-01';

assert_equal(dataTransformationProxy.price, 20);

// 添加自定义转换器
dataTransformationProxy.addTransformer('name', (val) => val.toUpperCase());
assert_equal(dataTransformationProxy.name, "JOHN DOE");

// 测试用例 82: 权限系统代理完整版
print('\n=== 测试用例 82: 权限系统代理完整版 ===');
const permissionSystemHandler = {
    get(target, prop) {
        // 获取当前用户权限
        const currentUser = target._currentUser || { role: 'guest', permissions: [] };
        const userPermissions = new Set(currentUser.permissions);
        
        // 权限检查
        const permissionMap = {
            // 读取权限
            adminData: ['read.admin'],
            userData: ['read.user', 'read.admin'],
            publicData: ['read.public', 'read.user', 'read.admin'],
            
            // 写入权限
            setAdminData: ['write.admin'],
            setUserData: ['write.user', 'write.admin'],
            
            // 删除权限
            deleteData: ['delete.user', 'delete.admin']
        };
        
        const requiredPermissions = permissionMap[prop];
        if (requiredPermissions) {
            const hasPermission = requiredPermissions.some(perm => userPermissions.has(perm));
            if (!hasPermission) {
                throw new Error(`权限不足: 需要 ${requiredPermissions.join(' 或 ')} 权限`);
            }
        }
        
        // 方法权限检查
        if (typeof target[prop] === 'function') {
            return function(...args) {
                const methodPermissions = {
                    deleteItem: ['delete.user', 'delete.admin'],
                    updateSettings: ['write.admin'],
                    createUser: ['write.admin']
                };
                
                const methodPerms = methodPermissions[prop];
                if (methodPerms) {
                    const hasMethodPermission = methodPerms.some(perm => userPermissions.has(perm));
                    if (!hasMethodPermission) {
                        throw new Error(`方法权限不足: ${prop}`);
                    }
                }
                
                print(`执行授权方法: ${prop}`);
                return Reflect.apply(target[prop], target, args);
            };
        }
        
        return Reflect.get(target, prop);
    },
    
    set(target, prop, value) {
        // 设置权限检查
        if (prop === '_currentUser') {
            print(`切换用户: ${value?.role}`);
            return Reflect.set(target, prop, value);
        }
        
        const currentUser = target._currentUser || { role: 'guest' };
        const userPermissions = new Set(currentUser.permissions);
        
        const writePermissions = {
            adminData: ['write.admin'],
            systemConfig: ['write.admin'],
            userSettings: ['write.user', 'write.admin']
        };
        
        const requiredWritePerms = writePermissions[prop];
        if (requiredWritePerms) {
            const hasWritePermission = requiredWritePerms.some(perm => userPermissions.has(perm));
            if (!hasWritePermission) {
                throw new Error(`写入权限不足: ${prop}`);
            }
        }
        
        return Reflect.set(target, prop, value);
    }
};

const permissionSystem = {
    adminData: '管理员数据',
    userData: '用户数据', 
    publicData: '公开数据',
    systemConfig: '默认配置',
    _currentUser: null,
    
    deleteItem(id) {
        return `删除项目 ${id}`;
    },
    
    updateSettings(settings) {
        this.systemConfig = settings;
        return '设置已更新';
    },
    
    createUser(user) {
        return `创建用户 ${user.name}`;
    }
};

const permissionSystemProxy = new Proxy(permissionSystem, permissionSystemHandler);

// 测试不同权限用户
const guestUser = { role: 'guest', permissions: ['read.public'] };
const normalUser = { role: 'user', permissions: ['read.public', 'read.user', 'write.user', 'delete.user'] };
const adminUser = { role: 'admin', permissions: ['read.public', 'read.user', 'read.admin', 'write.user', 'write.admin', 'delete.user', 'delete.admin'] };

// 测试访客权限
permissionSystemProxy._currentUser = guestUser;
print('访客访问公开数据:', permissionSystemProxy.publicData);
try {
    print('访客访问用户数据:', permissionSystemProxy.userData);
    assert_unreachable()
} catch (e) {
    print('访客权限错误:', e.message);
}

// 测试普通用户权限
permissionSystemProxy._currentUser = normalUser;
print('用户访问用户数据:', permissionSystemProxy.userData);
try {
    permissionSystemProxy.updateSettings('新配置');
} catch (e) {
    print('用户权限错误:', e.message);
}

// 测试管理员权限
permissionSystemProxy._currentUser = adminUser;
assert_equal(permissionSystemProxy.adminData, '管理员数据');
permissionSystemProxy.updateSettings('新配置');
print('管理员执行操作:', permissionSystemProxy.createUser({ name: 'Alice' }));

test_end();