/*
 * Copyright (c) 2024 Huawei Device Co., Ltd.
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
 * @tc.name:builtinsreflect
 * @tc.desc:test builtins reflect
 * @tc.type: FUNC
 * @tc.require: issueI8SXHD
 */
print("builtins reflect start");

// test1 -- reflect set length
var y = [];
Object.defineProperty(y, 1, { value: 42, configurable: false });
var tag1 = Reflect.set(y, 'length', 0);
var tag2 = Reflect.set(y, 'length', 5);
print(tag1);
print(tag2);

const v0 = 102630708;
let v55 = [];
let v56 = Object.create(v55)
Reflect.set(v56, "length", v0)
Reflect.set(v55, "length", v0, v56)
print("v56.length",v56.length)

var global = this;
var sym = Symbol("gaga");
var objects = [
    {},
    [],
    function() {},
    function() {
      return arguments;
    }(),
    Object(1),
    Object(true),
    Object('bla'),
    new Date,
    new RegExp,
    new Set,
    new Map,
    new WeakMap,
    new WeakSet,
    new ArrayBuffer(10),
    new Int32Array(5),
    Object,
    Function,
    Date,
    RegExp,
    global
];

function prepare(target) {
    target["bla"] = true;
    target[4] = 42;
    target[sym] = "foo";
    target["noconf"] = 43;
    Object.defineProperty(target, "noconf",
        { configurable: false });
    Object.defineProperty(target, "nowrite",
        { writable: false, configurable: true, value: 44 });
    Object.defineProperty(target, "getter",
        { get: function () {return this.bla}, configurable: true });
    Object.defineProperty(target, "setter",
        { set: function (x) {this.gaga = x}, configurable: true });
    Object.defineProperty(target, "setter2",
        { set: function (x) {}, configurable: true });
}

(function testReflectGetOnObject() {
    let i = 0;
    for (let target of objects) {
        prepare(target);
        if (true == Reflect.get(target, "bla") &&
            42 == Reflect.get(target, 4) &&
            42 == Reflect.get(target, "4") &&
            "foo" == Reflect.get(target, sym) &&
            43 == Reflect.get(target, "noconf") &&
            true == Reflect.get(target, "getter") &&
            undefined == Reflect.get(target, "setter") &&
            undefined == Reflect.get(target, "foo") &&
            undefined == Reflect.get(target, 333)) {
                print(i + "success_1");
        }
        let proto = target.__proto__;
        target.__proto__ = { get foo() {return this.bla} };
        if (true == Reflect.get(target, "foo")) {
            print(i + "success_2");
        }
        target.__proto__ = proto;
        i++;
    }
})();
let obj = {name:"tom"};
let pxobj = new Proxy(obj,{});
print(Reflect.get(pxobj,"name"))

{
    try {
        let arr=[1,2,3,4];
        arr.length=102600;
        Reflect.set(arr,"length","aaa",arr);
    } catch (error) {
        print(error.name)
    }
    try {
        let arr=[1,2,3,4];
        arr.length=102600;
        Reflect.set(arr,"length","aaa");
    } catch (error) {
        print(error.name)
    }
}

// reflect set ir
print("Reflect.set Ir test")
{
    // special test
    try {
        Reflect.set(undefined,"fail","fail")
    } catch (error) {
        print("Reflect.set undefined test pass")
    }

    try {
        Reflect.set(null,"fail","fail")
    } catch (error) {
        print("Reflect.set null test pass")
    }

    {
        let arrKey = [1,2,3]
        let testObj = {}
        Reflect.set(testObj,arrKey,"key is array")
        print(testObj[arrKey])
    }

    {
        let bigThenMaxKey = 2147483645;
        let testObj = {}
        Reflect.set(testObj,bigThenMaxKey,"key is over int32 max")
        print(testObj[bigThenMaxKey])
    }

    {
        let minusOne = -1;
        let testObj = {}
        Reflect.set(testObj,minusOne,"key is minusOne")
        print(testObj[minusOne])
    }

    {
        let lessThanInt32Min = -2147483648
        let testObj = {}
        Reflect.set(testObj,lessThanInt32Min,"key is lessThanInt32Min")
        print(testObj[lessThanInt32Min])
    }

    {
        let strKeyNumber = "5"
        let testObj = {}
        Reflect.set(testObj,strKeyNumber,"key is strKeyNumber")
        print(testObj[strKeyNumber])
    }

    {
        let strKeyNumberMinusOne = "-1"
        let testObj = {}
        Reflect.set(testObj,strKeyNumberMinusOne,"key is strKeyNumberMinusOne")
        print(testObj[strKeyNumberMinusOne])
    }

    {
        let strKeyNumberOverInt32 = "2147483645"
        let testObj = {}
        Reflect.set(testObj,strKeyNumberOverInt32,"key is strKeyNumberOverInt32")
        print(testObj[strKeyNumberOverInt32])
    }

    {
        let strKeyNumberLessInt32Min = "-2147483648"
        let testObj = {}
        Reflect.set(testObj,strKeyNumberLessInt32Min,"key is strKeyNumberLessInt32Min")
        print(testObj[strKeyNumberLessInt32Min])
    }

    {
        try {
            let objToStringThrowError = {
                toString(){
                    throw new Error("toString error")
                }
            }
            let testObj = {}
            Reflect.set(testObj,objToStringThrowError,"fail!")
        } catch (error) {
            print(error.message)
        }
    }

    {
        const testObj = {
            t: "origin"
        }
        const proxy = new Proxy(testObj,{})
        Reflect.set(proxy,"t","normal proxy")
        print(testObj["t"])
    }

    {
        const testObj = {
            t: "origin"
        }
        const proxy = new Proxy(testObj,{
            set(target,key,value){
                if (value === "nothrow proxy") {
                    target[key] = value
                    return true
                } else if (value === "return false proxy") {
                    target[key] = value
                    return false
                } else if (value === "throw proxy") {
                    target[key] = value
                    throw new Error("throw error in proxy")
                }

            }
        })
        Reflect.set(proxy,"t","return false proxy")
        print(testObj["t"])
        Reflect.set(proxy,"t","nothrow proxy")
        print(testObj["t"])
        try {
            Reflect.set(proxy,"t","throw proxy")
        } catch (error) {
            print(testObj["t"])
            print(error.message)
        }
    }

    {
        const dynamicHandler = new Proxy({}, {
            get(target, prop) {
              if (prop === "set") {
                return null;
              }
              return Reflect.get(target, prop);
            }
          });
          
          const target = {};
          const proxy = new Proxy(target, dynamicHandler);
          
          Reflect.set(proxy, "a", 1);
    }

    // SetPropertyByIndex
    {
        let tArr = new Uint8Array(1)
        let sym = Symbol("error symbol")
        try {
            Reflect.set(tArr,"1",sym)
        } catch (error) {
            print("sym to number fail")
        }
    }

    {
        let arr = []
        arr[1] = 1
        Object.preventExtensions(arr)
        Reflect.set(arr,"2",2)
        print("shoud not throw arr not Extensions")
    }

    {
        let arr = []
        arr[1] = 1
        Object.defineProperty(arr,"length",{writable:false})
        Reflect.set(arr,"5",2)
        print("don't throw error if length not writeable")
    }

    {
        let arr = []
        arr[1] = 1
        Object.defineProperty(arr,"2",{configurable:false})
        Reflect.set(arr,"2",2)
        print("don't throw error if value not configurable")
    }

    {
        let arr = []
        arr[0] = 5
        arr[1025] = 1025
        Object.defineProperty(arr,"length",{writable:false})
        Reflect.set(arr,"1026",1026)
        print("no exception or dictionary arr")
    }

    const setterNothrow = {
        set x(v){
            print("setterNothrow call setter")
            this.t = v
        }
    }
    const setterNothrowObj = Object.create(setterNothrow);
    Reflect.set(setterNothrowObj,"x","setterNothrow good")
    print(setterNothrowObj.t)

    const setterthrow = {
        set x(v){
            print("setterthrow call setter")
            throw new Error("error in setter")
        }
    }
    const setterthrowObj = Object.create(setterthrow);
    try {
        Reflect.set(setterthrowObj,"x","setterThrow good")
    } catch (error) {
        print(error.message)
    }

    const setterReturnFalse = {
        set x(v){
            print("setterReturnFalse call setter")
            return false
        }
    }
    const setterReturnFalseObj = Object.create(setterReturnFalse)
    Reflect.set(setterReturnFalseObj,"x","setterReturnFalse good");

    // normal test
    (function testSetProperty() {
        const obj = { foo: 42 };
        const result = Reflect.set(obj, "foo", 100);
        print("testSetProperty: ", obj.foo === 100 && result === true);
    })();


    (function testSetNonExistentProperty() {
        const obj = { foo: 42 };
        const result = Reflect.set(obj, "bar", 123);
        print("testSetNonExistentProperty: ", obj.bar === 123 && result === true);
    })();


    (function testSetInProxy() {
        const obj = { foo: 42 };
        const proxy = new Proxy(obj, {
            set(target, prop, value, receiver) {
                if (prop === "foo") {
                    target[prop] = value + 1; // 在 Proxy 中对 foo 做一些修改
                    return true;
                }
                return Reflect.set(target, prop, value, receiver);
            }
        });

        const result = Reflect.set(proxy, "foo", 100);
        print("testSetInProxy:", obj.foo === 101 && result === true);
    })();


    (function testSetWithSymbol() {
        const sym = Symbol("bar");
        const obj = {};
        const result = Reflect.set(obj, sym, 200);
        print("testSetWithSymbol:", obj[sym] === 200 && result === true);
    })();


    (function testSetReceiver() {
        const obj = { foo: 42 };
        const receiver = { foo: 100 };
        const result = Reflect.set(obj, "foo", 50, receiver);
        print("testSetReceiver:", obj.foo === 50 && receiver.foo === 100 && result === true);
    })();


    (function testSetPropertyAndCheckExists() {
        const obj = {};
        const result1 = Reflect.set(obj, "foo", 42);
        const result2 = Reflect.has(obj, "foo");
        print("testSetPropertyAndCheckExists:", result1 === true && result2 === true);
    })();


    (function testSetInProxyThrowsException() {
        const obj = { foo: 42 };
        const proxy = new Proxy(obj, {
            set(target, prop, value, receiver) {
                if (prop === "foo") {
                    throw new Error("Can't set foo!");
                }
                return Reflect.set(target, prop, value, receiver);
            }
        });

        try {
            Reflect.set(proxy, "foo", 100);
        } catch (e) {
            print("testSetInProxyThrowsException:", e.message === "Can't set foo!");
        }
    })();


    (function testSetThrowsError() {
        const obj = {};
        try {
            Reflect.set(obj, "foo", undefined);
            print("testSetThrowsError: undefined assignment allowed");
        } catch (e) {
            print("testSetThrowsError: Error while setting undefined");
        }
    })();


    (function testSetNestedObjectProperty() {
        const obj = { nested: { foo: 42 } };
        const result = Reflect.set(obj.nested, "foo", 100);
        print("testSetNestedObjectProperty:", obj.nested.foo === 100 && result === true);
    })();


    (function testSetNestedPropertyInProxy() {
        const obj = { nested: { foo: 42 } };
        const proxy = new Proxy(obj, {
            set(target, prop, value, receiver) {
                if (prop === "nested") {
                    target[prop].foo = value;
                    return true;
                }
                return Reflect.set(target, prop, value, receiver);
            }
        });

        const result = Reflect.set(proxy, "nested", { foo: 100 });
        print("testSetNestedPropertyInProxy:", obj.nested.foo === 100 && result === true);
    })();


    (function testSetNonConfigurableProperty() {
        const obj = {};
        Object.defineProperty(obj, "foo", {
            value: 42,
            writable: true,
            configurable: false
        });

        try {
            Reflect.set(obj, "foo", 100);
            print("testSetNonConfigurableProperty: No error for non-configurable");
        } catch (e) {
            print("testSetNonConfigurableProperty: Error for non-configurable property");
        }
    })();


    (function testSetPrototypeProperty() {
        const proto = { foo: 42 };
        const obj = Object.create(proto);
        const result = Reflect.set(obj, "foo", 100);
        print("testSetPrototypeProperty:", obj.foo === 100 && result === true);
    })();


    (function testSetAccessorPropertyInProxy() {
        const obj = {
            get foo() {
                return 42;
            },
            set foo(value) {
                this._foo = value;
            }
        };

        const proxy = new Proxy(obj, {
            set(target, prop, value, receiver) {
                if (prop === "foo") {
                    target[prop] = value + 1;
                    return true;
                }
                return Reflect.set(target, prop, value, receiver);
            }
        });

        const result = Reflect.set(proxy, "foo", 100);
        print("testSetAccessorPropertyInProxy:", obj._foo === 101 && result === true);
    })();
}

// reflect get ir test
print("Reflect.get Ir test")
{

    (function testGetterUsesReceiverThis() {
        const obj = {
            get val() {
                return this.x + 1;
            }
        };

        const receiver = { x: 10 };
        const result = Reflect.get(obj, "val", receiver);
        print("testGetterUsesReceiverThis:", result === 11);
    })();


    (function testReceiverIsNull() {
        const obj = {
            get val() {
                return this.x;
            }
        };

        try {
            Reflect.get(obj, "val", null);
        } catch (e) {
            print("testReceiverIsNull:", e instanceof TypeError);
        }
    })();


    (function testReceiverIsPrimitive() {
        const obj = {
            get val() {
                return this.length;
            }
        };

        const result = Reflect.get(obj, "val", "hello");
        print("testReceiverIsPrimitive:", result === 5);
    })();


    (function testReceiverMissingFields() {
        const obj = {
            get val() {
                return this.foo + 1;
            }
        };

        const receiver = {};
        const result = Reflect.get(obj, "val", receiver);
        print("testReceiverMissingFields:", Number.isNaN(result));
    })();


    (function testReceiverWithOwnFields() {
        const obj = {
            get val() {
                return this.foo * 2;
            }
        };

        const receiver = { foo: 21 };
        const result = Reflect.get(obj, "val", receiver);
        print("testReceiverWithOwnFields:", result === 42);
    })();


    (function testReceiverInheritance() {
        const base = {
            get foo() {
                return this.x;
            }
        };

        const receiver = Object.create({ x: 123 });
        const result = Reflect.get(base, "foo", receiver);
        print("testReceiverInheritance:", result === 123);
    })();


    (function testReceiverIsSameAsTarget() {
        const obj = {
            get x() {
                return this === obj;
            }
        };

        const result = Reflect.get(obj, "x", obj);
        print("testReceiverIsSameAsTarget:", result === true);
    })();


    (function testReceiverArrayLength() {
        const obj = {
            get lastIndex() {
                return this.length - 1;
            }
        };

        const result = Reflect.get(obj, "lastIndex", [1, 2, 3, 4]);
        print("testReceiverArrayLength:", result === 3);
    })();


    (function testReceiverIsProxy() {
        const obj = {
            get val() {
                return this.x;
            }
        };

        const proxy = new Proxy({ x: 42 }, {
            get(target, prop, receiver) {
                return Reflect.get(...arguments);
            }
        });

        const result = Reflect.get(obj, "val", proxy);
        print("testReceiverIsProxy:", result === 42);
    })();


    (function testReceiverIsClassInstance() {
        class MyClass {
            constructor(x) {
                this.x = x;
            }
            get val() {
                return this.x;
            }
        }

        const obj = new MyClass(100);
        const receiver = new MyClass(42);
        const result = Reflect.get(obj, "val", receiver);
        print("testReceiverIsClassInstance:", result === 42); // receiver 的 val 优先
    })();


    (function testReceiverIsInPrototypeChain() {
        const base = {
            get val() {
                return this.x;
            }
        };

        const proto = { x: 100 };
        const receiver = Object.create(proto);
        const result = Reflect.get(base, "val", receiver);
        print("testReceiverIsInPrototypeChain:", result === 100);
    })();


    (function testReceiverIsNull() {
        const obj = {
            get val() {
                return this.x;
            }
        };

        try {
            Reflect.get(obj, "val", null);
        } catch (e) {
            print("testReceiverIsNull:", e instanceof TypeError);
        }
    })();


    (function testReceiverIsUndefined() {
        const obj = {
            get val() {
                return this.x;
            }
        };

        try {
            Reflect.get(obj, "val", undefined);
        } catch (e) {
            print("testReceiverIsUndefined:", e instanceof TypeError);
        }
    })();


    (function testReceiverModifiedBySet() {
        const obj = {
            get val() {
                return this.x;
            }
        };

        const receiver = { x: 42 };
        Reflect.set(receiver, "x", 100);
        const result = Reflect.get(obj, "val", receiver);
        print("testReceiverModifiedBySet:", result === 100);
    })();

}

print("Reflect.has Ir test")
{
    (function testHasPropertyExists() {
        const obj = { foo: 42 };
        const result = Reflect.has(obj, "foo");
        print("testHasPropertyExists:", result === true);
    })();


    (function testHasPropertyDoesNotExist() {
        const obj = { foo: 42 };
        const result = Reflect.has(obj, "bar");
        print("testHasPropertyDoesNotExist:", result === false);
    })();


    (function testHasPropertyInPrototypeChain() {
        const proto = { foo: 42 };
        const obj = Object.create(proto);
        const result = Reflect.has(obj, "foo");
        print("testHasPropertyInPrototypeChain:", result === true);
    })();


    (function testHasWithNull() {
        try {
            Reflect.has(null, "foo");
        } catch (e) {
            print("testHasWithNull:", e instanceof TypeError);
        }
    })();

    (function testHasWithUndefined() {
        try {
            Reflect.has(undefined, "foo");
        } catch (e) {
            print("testHasWithUndefined:", e instanceof TypeError);
        }
    })();


    (function testHasInProxy() {
        const obj = { foo: 42 };
        const proxy = new Proxy(obj, {
            has(target, prop) {
                return prop === "foo";
            }
        });

        const result1 = Reflect.has(proxy, "foo");
        const result2 = Reflect.has(proxy, "bar");
        print("testHasInProxy: foo exists:", result1 === true);
        print("testHasInProxy: bar exists:", result2 === false);
    })();


    (function testHasAndGetCombination() {
        const obj = { foo: 42 };

        const hasFoo = Reflect.has(obj, "foo");
        const getFoo = hasFoo ? Reflect.get(obj, "foo") : undefined;
        print("testHasAndGetCombination:", getFoo === 42);
    })();


    (function testHasInProxyWithNonExistentProperty() {
        const obj = {};
        const proxy = new Proxy(obj, {
            has(target, prop) {
                return prop === "foo";
            }
        });

        const result = Reflect.has(proxy, "bar");
        print("testHasInProxyWithNonExistentProperty:", result === false);
    })();


    (function testHasVsInOperator() {
        const obj = { foo: 42 };

        const resultReflect = Reflect.has(obj, "foo");
        const resultIn = "foo" in obj;
        print("testHasVsInOperator:", resultReflect === resultIn);
    })();


    (function testHasWithSymbol() {
        const sym = Symbol("bar");
        const obj = { [sym]: 123 };

        const result = Reflect.has(obj, sym);
        print("testHasWithSymbol:", result === true);
    })();


    (function testHasWithNonExistentSymbol() {
        const sym = Symbol("bar");
        const obj = { foo: 42 };

        const result = Reflect.has(obj, sym);
        print("testHasWithNonExistentSymbol:", result === false);
    })();
}

print("builtins reflect end");
