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

// Test map Symbol.iterator change.
{
    let mapPrototype = Object.getPrototypeOf(new Map());
    let iteratorSymbol = Symbol.iterator;

    function customMapIterator() {
        let mapEntries = this.entries();
        let entryIndex = 0;

        return {
            next: function () {
                if (entryIndex < 2) {
                    let entry = mapEntries.next();
                    entryIndex++;
                    // 这里只返回键，与标准的返回键值对不同
                    return { done: false, value: entry.value[0] };
                }
                return { done: true, value: undefined };
            },
            customProperty: "This is a custom property for Map iterator"
        };
    }

    mapPrototype[iteratorSymbol] = customMapIterator;
    let myMap = new Map([['key1', 'value1'], ['key2', 'value2']]);
    print("Testing methods affected by map Symbol.iterator:");
    for (let item of myMap) {
        print(item);
    }
}

// Test set Symbol.iterator change.
{
    let setPrototype = Object.getPrototypeOf(new Set());
    let iteratorSymbol = Symbol.iterator;

    function customSetIterator() {
        let setValues = this.values();
        let valueIndex = 0;

        return {
            next: function () {
                if (valueIndex < 2) {
                    print(valueIndex);
                    let value = setValues.next();
                    valueIndex++;
                    // 将元素重复两次返回，与标准不同
                    return { done: false, value: [value.value, value.value] };
                }
                return { done: true, value: undefined };
            },
            customProperty: "This is a custom property for Set iterator"
        };
    }

    setPrototype[iteratorSymbol] = customSetIterator;
    let mySet = new Set(['value1', 'value2']);

    print("Testing methods affected by set Symbol.iterator:");
    for (let item of mySet) {
        print(item);
    }
}

// Test array Symbol.iterator change.
{
    let arrayPrototype = Object.getPrototypeOf([]);
    let iteratorSymbol = Symbol.iterator;

    function customArrayIterator() {
        let array = this;
        let index = 0;

        return {
            next: function () {
                if (index < array.length) {
                    // 倒序返回数组元素，与标准顺序不同
                    let value = array[array.length - 1 - index];
                    index++;
                    return { done: false, value: value };
                }
                return { done: true, value: undefined };
            },
            customProperty: "This is a custom property for Array iterator"
        };
    }

    arrayPrototype[iteratorSymbol] = customArrayIterator;
    let myArray = ['value1', 'value2'];

    print("Testing methods affected by array Symbol.iterator:");
    for (let item of myArray) {
        print(item);
    }
}

// -----------------------Test inheritance---------------------------------

// Test map Symbol.iterator change using inheritance (new implementation).
{
    class CustomMapInherit extends Map {
        constructor(initEntries) {
            super(initEntries);
        }
    };
    function customMapIterator() {
        let mapEntries = this.entries();
        let entryIndex = 0;

        return {
            next: function () {
                if (entryIndex < 2) {
                    print("CustomMapInherit");
                    let entry = mapEntries.next();
                    entryIndex++;
                    // 这里只返回键，与标准的返回键值对不同
                    return { done: false, value: entry.value[0] };
                }
                return { done: true, value: undefined };
            },
            customProperty: "This is a custom property for CustomMapInherit iterator"
        };
    }
    let CustomMapPrototype = Object.getPrototypeOf(new CustomMapInherit());
    let iteratorSymbol = Symbol.iterator;
    CustomMapPrototype[iteratorSymbol] = customMapIterator;
    print("Testing methods affected by map Symbol.iterator (inherited way):");
    let customMap = new CustomMapInherit([['key1', 'value1'], ['key2', 'value2']]);
    for (let item of customMap) {
        print(item);
    }
}

// Test set Symbol.iterator change using inheritance (new implementation).
{
    class CustomSetInherit extends Set {
        constructor(initValues) {
            super(initValues);
        }
    }
    function customSetIterator() {
        let setValues = this.values();
        let valueIndex = 0;

        return {
            next: function () {
                if (valueIndex < 2) {
                    print("CustomSetInherit");
                    let value = setValues.next();
                    valueIndex++;
                    // 将元素重复两次返回，与标准不同（这里可按需调整返回逻辑，保持或修改重复行为等）
                    return { done: false, value: [value.value, value.value] };
                }
                return { done: true, value: undefined };
            },
            customProperty: "This is a custom property for CustomSetInherit iterator"
        };
    }
    let CustomSetPrototype = Object.getPrototypeOf(new CustomSetInherit());
    let iteratorSymbol = Symbol.iterator;
    CustomSetPrototype[iteratorSymbol] = customSetIterator;
    print("Testing methods affected by custom Set Symbol.iterator (inherited way):");
    let customSet = new CustomSetInherit(['value1', 'value2']);
    for (const item of customSet) {
        print(item);
    }
}

// Test array Symbol.iterator change using inheritance (new implementation).
{
    class CustomArrayInherit extends Array {
        constructor(...args) {
            super(...args);
        }
    }
    function customArrayIterator() {
        let array = this;
        let index = 0;

        return {
            next: function () {
                if (index < array.length) {
                    print("CustomArrayInherit");
                    let value = array[array.length - 1 - index];
                    index++;
                    return { done: false, value: value };
                }
                return { done: true, value: undefined };
            },
            customProperty: "This is a custom property for CustomArrayInherit iterator"
        };
    }
    let CustomArrayPrototype = Object.getPrototypeOf(new CustomArrayInherit());
    let iteratorSymbol = Symbol.iterator;
    CustomArrayPrototype[iteratorSymbol] = customArrayIterator;
    print("Testing methods affected by custom Array Symbol.iterator (inherited way):");
    let customArray = new CustomArrayInherit('value1', 'value2');
    for (const item of customArray) {
        print(item);
    }
}

// This case aims to test the logic which check the undefined result of GetIterator.
{
    class c2 extends Object {}
    Array.prototype[Symbol.iterator] = function () {};
    try {
        let myC2 = new c2();
    } catch (error) {
        print(error);
    }
}