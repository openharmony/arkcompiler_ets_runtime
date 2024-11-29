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