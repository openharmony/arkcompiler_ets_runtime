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
 * @tc.name:sendablearray
 * @tc.desc:test sendablearray
 * @tc.type: FUNC
 * @tc.require: issueI8QUU0
 */

// @ts-nocheck
declare function print(str: any): string;

class SuperClass {
    public num: number = 0;
    constructor(num: number) {
        "use sendable"
        this.num = num;
    }
}

class SubClass extends SuperClass {
    public strProp: string = "";
    constructor(num: number) {
        "use sendable"
        super(num);
        this.strProp = "" + num;
    }
}

class SubSharedClass extends SendableArray {
  constructor() {
    'use sendable';
    super();
  }
}

class SuperUnSharedClass {
    public num: number = 0;
    constructor(num: number) {
        this.num = num;
    }
}

class SubNormalArray extends Array {
    constructor() {
        super();
    }
}

SendableArray.from<string>(['a', 'r', 'k']);

function at() {
    print("Start Test at")
    const array1 = new SendableArray<number>(5, 12, 8, 130, 44);
    let index = 2;
    print(`An index of ${index} returns ${array1.at(index)}`); // An index of 2 returns 8

    index = -2;
    print(`An index of ${index} returns ${array1.at(index)}`); // An index of -2 returns 130

    index = 200;
    print(`An index of ${index} returns ${array1.at(index)}`); // An index of 200 returns undefined

    print(`An index of null returns ${array1.at(null)}`); // An index of null returns 5

    print(`An index of undefined returns ${array1.at(undefined)}`); // An index of undefined returns 5

    print(`An index of undefined returns ${array1.at(true)}`); // An index of true returns 12

    print(`An index of undefined returns ${array1.at(false)}`); // An index of false returns 5

    index = 2871622679;
    print(`An index of 2871622679 returns ${array1.at(index)}`); // An index of 2871622679 returns undefined
}

function entries() {
    print("Start Test entries")
    const array1 = new SendableArray<string>('a', 'b', 'c');
    const iterator = array1.entries();
    for (const [key, value] of iterator) {
        print("" + key + "," + value); // 0 a, 1 b, 2 c
    }
}

function keys() {
    print("Start Test keys")
    const array1 = new SendableArray<string>('a', 'b', 'c');
    const iterator = array1.keys();
    for (const key of iterator) {
        print("" + key); // 0, 1, 2
    }
}

function values() {
    print("Start Test values")
    const array1 = new SendableArray<string>('a', 'b', 'c');
    const iterator = array1.values();
    for (const value of iterator) {
        print("" + value); // a, b, c
    }
}

function find() {
    print("Start Test find")
    const array1 = new SendableArray<number>(5, 12, 8, 130, 44);

    const found = array1.find((element: number) => element > 10);
    print("" + found); // 12

    const array2 = new SendableArray<SuperClass>(
      new SubClass(5),
      new SubClass(32),
      new SubClass(8),
      new SubClass(130),
      new SubClass(44),
    );
    const result: SubClass | undefined = array2.find<SubClass>(
      (value: SuperClass, index: number, obj: SendableArray<SuperClass>) => value instanceof SubClass,
    );
    print((new SubClass(5)).strProp); // 5
}

function includes() {
    print("Start Test includes")
    const array1 = new SendableArray<number>(1, 2, 3);
    print("" + array1.includes(2)); // true

    const pets = new SendableArray<string>('cat', 'dog', 'bat');
    print("" + pets.includes('cat')); // true

    print("" + pets.includes('at')); // false
}

function containsAll() {
    print("Start Test containsAll")

    print("=== Common usage case ===");
    const array1 = new SendableArray<number>(1, 2, 3);
    const normalArray1 = new Array<number>(1, 3);
    print("" + array1.containsAll([1, 3]));
    print("" + array1.containsAll(normalArray1));
    print("" + array1.containsAll(new SendableArray<number>(1, 3)));

    const normalArray2 = new Array<number>(1, 3, 5);
    print("" + array1.containsAll([6]));
    print("" + array1.containsAll(normalArray2));
    print("" + array1.containsAll(new SendableArray<number>(1, 2, 3, 4)));
    const normalArray3 = new Array([1, 3]); // [[1, 3]]
    print("" + array1.containsAll(normalArray3));

    print("" + array1.containsAll([1, 2, 3]));
    print("" + array1.containsAll(new SendableArray<number>(1, 2, 3)));
    print("" + array1.containsAll([3, 1]));
    print("" + array1.containsAll(new SendableArray<number>(3, 1)));

    print("=== empty elements case ===");
    print("" + array1.containsAll(new SendableArray<number>()));
    print("" + array1.containsAll([]));

    print("=== empty receiver case ===");
    const emptyArray = new SendableArray<number>();
    print("" + emptyArray.containsAll([1]));
    print("" + emptyArray.containsAll(new SendableArray<number>(1)));
    print("" + emptyArray.containsAll([]));
    print("" + emptyArray.containsAll(new SendableArray<number>()));
    print("" + emptyArray.containsAll(new Array<number>(1)));
    print("" + emptyArray.containsAll(new Array<number>(10)));

    print("=== repeating elements case ===");
    print("" + array1.containsAll([3, 3, 2, 2]));
    print("" + array1.containsAll(new SendableArray<number>(3, 3, 2, 2)));

    print("=== NAN/+0/-0 case ===");
    const sameValueZeroArray1 = new SendableArray<number>(NaN, +0);
    print("" + sameValueZeroArray1.containsAll([NaN, -0]));
    print("" + sameValueZeroArray1.containsAll(new SendableArray<number>(NaN, -0)));
    print("" + sameValueZeroArray1.containsAll([0]));
    print("" + sameValueZeroArray1.containsAll(new SendableArray<number>(0)));
    const sameValueZeroArray2 = new SendableArray<number>(-0);
    print("" + sameValueZeroArray2.containsAll([+0]));
    print("" + sameValueZeroArray2.containsAll(new SendableArray<number>(+0)));
    print("" + sameValueZeroArray2.containsAll([NaN]));
    print("" + sameValueZeroArray2.containsAll(new SendableArray<number>(NaN)));
    const sameValueZeroArray3 = new SendableArray<number>(0);
    print("" + sameValueZeroArray3.containsAll([-0]));
    print("" + sameValueZeroArray3.containsAll(new SendableArray<number>(-0)));
    print("" + sameValueZeroArray3.containsAll([+0]));
    print("" + sameValueZeroArray3.containsAll(new SendableArray<number>(+0)));

    print("=== string case ===");
    const numberArray = new SendableArray<number | string>(1);
    print("" + numberArray.containsAll(["1"]));
    print("" + numberArray.containsAll(new SendableArray<number>("1")));

    print("=== prototype modify case ===");
    const undefinedArray = new SendableArray<undefined>(undefined);
    print("" + undefinedArray.containsAll(new Array<undefined>(1)));
    print("" + undefinedArray.containsAll(new SendableArray<undefined>(undefined)));
    print("" + array1.containsAll(new SendableArray<number>(undefined)));
    print("" + array1.containsAll(new Array<number>(1)));
    Array.prototype[10] = 10; // proto change, 10 do not contain in array1
    Array.prototype[0] = 30; // proto change, 30 do not contain in array1
    try {
        print("" + array1.containsAll(new Array<number>(1))); // [hole]
    } finally {
        delete Array.prototype[10];
        delete Array.prototype[0];
    }

    print("=== accessor elements case ===");
    const getterArray = new Array<number>(1);
    Object.defineProperty(getterArray, 0, {
        get() {
            return 2;
        }
    });
    print("" + array1.containsAll(getterArray));
    const getterArray2 = new Array<number>(0);
    Object.defineProperty(getterArray2, 0, {
        get() {
            return 2;
        }
    }); // [2]
    print("" + array1.containsAll(getterArray2));
    Object.defineProperty(getterArray2, 1, {
        get() {
            return 3;
        }
    }); // [2, 3]
    print("" + array1.containsAll(getterArray2));
    Object.defineProperty(getterArray2, 3, {
        get() {
            return 1;
        }
    }); // [2, 3, , 1]
    print("" + array1.containsAll(getterArray2));

    print("=== sparseArray case ===");
    let sparseArray = new Array(2);
    sparseArray[0] = 1;
    print("" + array1.containsAll(sparseArray));
    sparseArray = new Array(0x4000);
    sparseArray[1050] = 3;
    sparseArray[2050] = 3;
    sparseArray[3050] = 1;
    print("" + array1.containsAll(sparseArray));
    const arrayWithUndefined = new SendableArray<number | undefined>(1, 3, undefined);
    print("" + arrayWithUndefined.containsAll(sparseArray));
    sparseArray = new Array(2);
    print("" + arrayWithUndefined.containsAll(sparseArray));
    sparseArray = new Array(3);
    sparseArray[1] = 1;
    print("" + arrayWithUndefined.containsAll(sparseArray));

    print("=== undefined/null case ===");
    const nullAndUndefinedArray = new SendableArray<null | undefined>(null, undefined);
    const onlyNullArray = new SendableArray<null | undefined>(null);
    const onlyUndefinedArray = new SendableArray<null | undefined>(undefined);
    print("" + nullAndUndefinedArray.containsAll([null]) + "," + nullAndUndefinedArray.containsAll([undefined]));
    print("" + nullAndUndefinedArray.containsAll(new SendableArray<null>(null)) +
        "," + nullAndUndefinedArray.containsAll(new SendableArray<null>(undefined)));
    print("" + onlyNullArray.containsAll([undefined]) + "," + onlyUndefinedArray.containsAll([null]));
    print("" + onlyNullArray.containsAll(new SendableArray<null>(undefined)) +
        "," + onlyUndefinedArray.containsAll(new SendableArray<null>(null)));

    print("=== obj case ===");
    const obj1 = new SuperClass(1);
    const obj2 = new SuperClass(1);
    const objArray = new SendableArray<SuperClass>(obj1);
    print("" + objArray.containsAll([obj1]));
    print("" + objArray.containsAll(new SendableArray<SuperClass>(obj1)));
    print("" + objArray.containsAll([obj2]));
    print("" + objArray.containsAll(new SendableArray<SuperClass>(obj2)));
    const unshared = new SuperUnSharedClass(1);
    print("" + objArray.containsAll([unshared]));

    print("=== inherit case start ===");
    const subSharedReceiver = new SubSharedClass();
    subSharedReceiver.push(1);
    subSharedReceiver.push(2);
    subSharedReceiver.push(3);
    const subSharedElements = new SubSharedClass();
    subSharedElements.push(1);
    subSharedElements.push(3);
    const subNormalArray = new SubNormalArray();
    subNormalArray.push(1);
    subNormalArray.push(3);
    print("" + subSharedReceiver.containsAll([1, 3]));
    print("" + subSharedReceiver.containsAll(new SendableArray<number>(1, 3)));
    print("" + subSharedReceiver.containsAll(subNormalArray));
    print("" + subSharedReceiver.containsAll(subSharedElements));
    print("" + array1.containsAll(subSharedElements));
    print("" + array1.containsAll(subNormalArray));

    print("=== immutableArray case ===");
    const immutableArray = new SendableArray<number>(1, 2);
    const before = "" + immutableArray;
    const result = immutableArray.containsAll([2]);
    print("" + result + "," + (immutableArray.length === 2) + "," + (before === "" + immutableArray));
    const elementsArray = [2];
    const elementsBefore = "" + elementsArray;
    const elementsResult = immutableArray.containsAll(elementsArray);
    print("" + elementsResult + "," + (elementsArray.length === 1) + "," + (elementsBefore === "" + elementsArray));
    const elementsSharedArray = new SendableArray<number>(2);
    const elementsSharedBefore = "" + elementsSharedArray;
    const sharedElementsResult = immutableArray.containsAll(elementsSharedArray);
    print("" + sharedElementsResult + "," + (elementsSharedArray.length === 1) +
        "," + (elementsSharedBefore === "" + elementsSharedArray));

    print("=== receiver self test ===");
    print("" + array1.containsAll(array1));
    print("" + subSharedReceiver.containsAll(subSharedReceiver));

    print("=== concurrent case ===");
    const containsAllConcurrentArray = new SendableArray<number>(1, 2);
    try {
        print("" + containsAllConcurrentArray.retainAll((_value: number) => {
            containsAllConcurrentArray.containsAll(containsAllConcurrentArray);
            return true;
        }));
    } catch (err) {
        print("containsAll self concurrent failed. err: " + err + ", code: " + err.code);
    }
    const containsAllConcurrentArray2 = new SendableArray<number>(1, 2);
    try {
        containsAllConcurrentArray2.forEach((_value: number, _index: number, array: SendableArray<number>) => {
            print(array.containsAll(containsAllConcurrentArray2));
        });
    } catch (err) {
        print("containsAll self concurrent failed. err: " + err + ", code: " + err.code +
            ", array: " + containsAllConcurrentArray2);
    }

    print("=== exception case ===");
    try {
        print("" + array1.containsAll({0: 1, length: 1}));
    } catch (err) {
        print("containsAll arrayLike failed. err: " + err + ", code: " + err.code);
    }

    try {
        print("" + array1.containsAll());
    } catch (err) {
        print("containsAll undefined failed. err: " + err + ", code: " + err.code);
    }

    try {
        print("" + array1.containsAll(undefined));
    } catch (err) {
        print("containsAll undefined failed. err: " + err + ", code: " + err.code);
    }

    try {
        print("" + array1.containsAll(null));
    } catch (err) {
        print("containsAll null failed. err: " + err + ", code: " + err.code);
    }

    try {
        print("" + array1.containsAll(new Uint8Array([1, 2])));
    } catch (err) {
        print("containsAll typedArray failed. err: " + err + ", code: " + err.code);
    }

    try {
        print("" + array1.containsAll(new Set([1])));
    } catch (err) {
        print("containsAll set failed. err: " + err + ", code: " + err.code);
    }

    try {
        print("" + array1.containsAll(new SendableSet([1])));
    } catch (err) {
        print("containsAll sendableSet failed. err: " + err + ", code: " + err.code);
    }

    try {
        print("" + array1.containsAll({}));
    } catch (err) {
        print("containsAll object failed. err: " + err + ", code: " + err.code);
    }

    try {
        print("" + array1.containsAll(1));
    } catch (err) {
        print("containsAll primitive failed. err: " + err + ", code: " + err.code);
    }

    try {
        print("" + array1.containsAll(new Map([[1, 'one']]).entries()));
    } catch (err) {
        print("containsAll iterator failed. err: " + err + ", code: " + err.code);
    }

    const unboundContainsAll = array1.containsAll;
    const boundContainsAll = unboundContainsAll.bind(new SuperUnSharedClass(1));
    try {
        print("" + boundContainsAll([1]));
    } catch (err) {
        print("containsAll bind failed. err: " + err + ", code: " + err.code);
    }
}

function retainAll() {
    print("Start Test retainAll");

    print("=== retainAll common usage case ===");
    const array1 = new SendableArray<number>(1, 2, 3);
    print("" + array1.retainAll([1, 2, 3]) + "," + array1);
    print("" + array1.retainAll(new SendableArray<number>(1, 2, 3)) + "," + array1);
    const partialArray1 = new SendableArray<number>(1, 2, 3, 4);
    print("" + partialArray1.retainAll([1, 3]) + "," + partialArray1);
    const partialArray2 = new SendableArray<number>(3, 1, 2);
    print("" + partialArray2.retainAll(new SendableArray<number>(1, 3)) + "," + partialArray2);
    const clearArray1 = new SendableArray<number>(1, 2);
    print("" + clearArray1.retainAll([6]) + "," + clearArray1.length + "," + clearArray1);
    const clearArray2 = new SendableArray<number>(1, 2);
    print("" + clearArray2.retainAll(new SendableArray<number>(6)) + "," + clearArray2.length + "," + clearArray2);

    print("=== retainAll empty case ===");
    const emptyElementsArray1 = new SendableArray<number>(1, 2);
    print("" + emptyElementsArray1.retainAll([]) + "," + emptyElementsArray1.length + "," + emptyElementsArray1);
    const emptyElementsArray2 = new SendableArray<number>(1, 2);
    print("" + emptyElementsArray2.retainAll(new SendableArray<number>()) +
        "," + emptyElementsArray2.length + "," + emptyElementsArray2);
    const emptyElementsArray3 = new SendableArray<number>(1, 2);
    print("" + emptyElementsArray3.retainAll(new Array<number>(10)) +
        "," + emptyElementsArray3.length + "," + emptyElementsArray3);
    const emptyReceiver = new SendableArray<number>();
    print("" + emptyReceiver.retainAll([1]) + "," + emptyReceiver.length + "," + emptyReceiver);
    print("" + emptyReceiver.retainAll(new SendableArray<number>(1)) + "," +
        emptyReceiver.length + "," + emptyReceiver);
    print("" + emptyReceiver.retainAll([]) + "," + emptyReceiver.length + "," + emptyReceiver);
    print("" + emptyReceiver.retainAll(new SendableArray<number>()) + "," +
        emptyReceiver.length + "," + emptyReceiver);
    print("" + emptyReceiver.retainAll(new Array<number>()) + "," + emptyReceiver.length + "," + emptyReceiver);
    print("" + emptyReceiver.retainAll(new Array<number>(10)) + "," + emptyReceiver.length + "," + emptyReceiver);

    print("=== retainAll repeating elements case ===");
    const repeatedElementsArray = new SendableArray<number>(1);
    print("" + repeatedElementsArray.retainAll([1, 1]) + "," + repeatedElementsArray);
    const repeatedReceiverArray = new SendableArray<number>(1, 1, 2, 3);
    print("" + repeatedReceiverArray.retainAll(new SendableArray<number>(1, 3, 3)) + "," + repeatedReceiverArray);

    print("=== retainAll SameValueZero case ===");
    const sameValueZeroArray1 = new SendableArray<number>(NaN, 1);
    print("" + sameValueZeroArray1.retainAll([NaN]) + "," + sameValueZeroArray1);
    const sameValueZeroArray2 = new SendableArray<number>(+0, 1);
    print("" + sameValueZeroArray2.retainAll(new SendableArray<number>(-0)) + "," + sameValueZeroArray2);
    const sameValueZeroArray3 = new SendableArray<number>(-0, 1);
    print("" + sameValueZeroArray3.retainAll([+0]) + "," + sameValueZeroArray3);
    const sameValueZeroArray4 = new SendableArray<number>(0, 1);
    print("" + sameValueZeroArray4.retainAll(new SendableArray<number>(-0)) + "," + sameValueZeroArray4);
    const sameValueZeroArray5 = new SendableArray<number>(0, -0);
    print("" + sameValueZeroArray5.retainAll(new SendableArray<number>(+0)) + "," + sameValueZeroArray5);

    print("=== retainAll undefined/null case ===");
    const nullAndUndefinedArray1 = new SendableArray<null | undefined>(null, undefined);
    print("" + nullAndUndefinedArray1.retainAll([undefined]) + "," + nullAndUndefinedArray1.length +
        "," + (nullAndUndefinedArray1[0] === undefined));
    const nullAndUndefinedArray2 = new SendableArray<null | undefined>(null, undefined);
    print("" + nullAndUndefinedArray2.retainAll(new SendableArray<null | undefined>(null)) +
        "," + nullAndUndefinedArray2.length + "," + (nullAndUndefinedArray2[0] === null));

    print("=== retainAll sparseArray case ===");
    const undefinedArray = new SendableArray<undefined>(undefined);
    print("" + undefinedArray.retainAll(new Array<undefined>(1)) + "," + undefinedArray.length +
        "," + undefinedArray);
    const numberArray = new SendableArray<number>(1);
    print("" + numberArray.retainAll(new Array<number>(1)) + "," + numberArray.length + "," + numberArray);
    const noProtoArray = new SendableArray<number>(1);
    Array.prototype[0] = 1;
    try {
        print("" + noProtoArray.retainAll(new Array<number>(1)) + "," + noProtoArray.length + "," + noProtoArray);
    } finally {
        delete Array.prototype[0];
    }
    const sparseReceiver = new SendableArray<number | undefined>(1, 3, undefined);
    const sparseElements = new Array(3);
    sparseElements[1] = 3;
    print("" + sparseReceiver.retainAll(sparseElements) + "," + sparseReceiver.length + "," + sparseReceiver);
    const getterElements = new Array<number>(1);
    Object.defineProperty(getterElements, 0, {
        get() {
            return 2;
        }
    });
    Object.defineProperty(getterElements, 10, {
        get() {
            return 11;
        }
    });
    const getterReceiver = new SendableArray<number>(1, 2, 3);
    print("" + getterReceiver.retainAll(getterElements) + "," + getterReceiver.length + "," + getterReceiver);
    const getterEmptyElements = new Array<number>(0);
    Object.defineProperty(getterEmptyElements, 2, {
        get() {
            return 3;
        }
    }); // [, , 3]
    const getterReceiver2 = new SendableArray<number>(1, 2, 3);
    print("" + getterReceiver2.retainAll(getterEmptyElements) + "," + getterReceiver2.length + "," + getterReceiver2);
    let sparseArray = new Array(0x4000);
    sparseArray[1050] = 3;
    sparseArray[2050] = 3;
    sparseArray[3050] = 1;
    const sparseReceiver2 = new SendableArray<number>(1, 2, 3);
    print("" + sparseReceiver2.retainAll(sparseArray) + "," + sparseReceiver2.length + "," + sparseReceiver2);

    print("=== retainAll obj case ===");
    const obj1 = new SuperClass(1);
    const obj2 = new SuperClass(1);
    const objArray1 = new SendableArray<SuperClass>(obj1);
    print("" + objArray1.retainAll([obj1]) + "," + objArray1.length + "," + objArray1[0].num);
    const objArray2 = new SendableArray<SuperClass>(obj1);
    print("" + objArray2.retainAll(new SendableArray<SuperClass>(obj2)) + "," + objArray2.length + "," + objArray2);
    const unshared = new SuperUnSharedClass(1);
    const objArray3 = new SendableArray<number>(1, 2);
    print("" + objArray3.retainAll([unshared]) + "," + objArray3.length + "," + objArray3);

    print("=== retainAll inherit case ===");
    const subSharedReceiver = new SubSharedClass();
    subSharedReceiver.push(1);
    subSharedReceiver.push(2);
    subSharedReceiver.push(3);
    const subSharedElements = new SubSharedClass();
    subSharedElements.push(1);
    subSharedElements.push(3);
    const subNormalElements = new SubNormalArray();
    subNormalElements.push(1);
    subNormalElements.push(3);
    print("" + subSharedReceiver.retainAll(subNormalElements) + "," + subSharedReceiver);
    const subSharedReceiver2 = new SubSharedClass();
    subSharedReceiver2.push(1);
    subSharedReceiver2.push(2);
    subSharedReceiver2.push(3);
    print("" + subSharedReceiver2.retainAll(subSharedElements) + "," + subSharedReceiver2);
    let sharedReceiver1 = new SendableArray<number>(1, 2, 3);
    print("" + sharedReceiver1.retainAll(subNormalElements) + "," + sharedReceiver1);
    let sharedReceiver2 = new SendableArray<number>(1, 2, 3);
    print("" + sharedReceiver2.retainAll(subSharedElements) + "," + sharedReceiver2);

    print("=== retainAll immutable elements case ===");
    const immutableReceiver = new SendableArray<number>(1, 2);
    const normalElements = [2];
    const normalElementsBefore = "" + normalElements;
    print("" + immutableReceiver.retainAll(normalElements) + "," + immutableReceiver +
        "," + (normalElements.length === 1) + "," + (normalElementsBefore === "" + normalElements));
    const sharedElements = new SendableArray<number>(2);
    const sharedElementsBefore = "" + sharedElements;
    print("" + immutableReceiver.retainAll(sharedElements) + "," + immutableReceiver +
        "," + (sharedElements.length === 1) + "," + (sharedElementsBefore === "" + sharedElements));

    print("=== retainAll predicate case ===");
    const predicateArray0 = new SendableArray<number>(1, 2, 3);
    const predicateResult0 = predicateArray0.retainAll((value: number) => {
        return false;
    });
    print("" + predicateResult0 + "," + predicateArray0);
    const predicateArray1 = new SendableArray<number>(1, 2, 3);
    let predicateCalls = "";
    const predicateResult1 = predicateArray1.retainAll((value: number) => {
        predicateCalls += value + ";";
        return value >= 2;
    });
    print("" + predicateResult1 + "," + predicateArray1 + "," + predicateCalls);
    const predicateArray1SingleArg = new SendableArray<number>(1, 2);
    let predicateSingleArgCalls = "";
    const predicateSingleArgResult = predicateArray1SingleArg.retainAll(function (value: number): boolean {
        predicateSingleArgCalls += arguments.length + ":" + value + ";";
        return true;
    });
    print("" + predicateSingleArgResult + "," + predicateArray1SingleArg + "," + predicateSingleArgCalls);
    const predicateArray2 = new SendableArray<number>(1, 2);
    print("" + predicateArray2.retainAll((value: number) => value > 0) + "," + predicateArray2);
    const predicateArray3 = new SendableArray<number>(1, 2);
    print("" + predicateArray3.retainAll((value: number) => value > 3) + "," + predicateArray3.length +
        "," + predicateArray3);
    const predicateArray4 = new SendableArray<number>(1, 1, 2);
    print("" + predicateArray4.retainAll((value: number) => value === 1) + "," + predicateArray4);
    const predicateArray5 = new SendableArray<number>(1, 2, 3);
    print("" + predicateArray5.retainAll((value: number) => value === 2 ? "keep" : "") + "," + predicateArray5);
    const predicateArray6 = new SendableArray<number>(1, 2, 3);
    print("" + predicateArray6.retainAll((value: number) => value === 2 ? 1 : 0) + "," + predicateArray6);
    const predicateArray7 = new SendableArray<number>(1, 2, 3);
    print("" + predicateArray7.retainAll((value: number) => value === 2 ? {} : null) + "," + predicateArray7);
    const emptyPredicateArray = new SendableArray<number>();
    let emptyPredicateCalls = 0;
    print("" + emptyPredicateArray.retainAll(() => {
        emptyPredicateCalls++;
        return true;
    }) + "," + emptyPredicateArray.length + "," + emptyPredicateCalls);
    const subPredicateReceiver = new SubSharedClass();
    subPredicateReceiver.push(1);
    subPredicateReceiver.push(2);
    subPredicateReceiver.push(3);
    print("" + subPredicateReceiver.retainAll((value: number) => value !== 2) + "," + subPredicateReceiver);
    const predicateThrowArray = new SendableArray<number>(1, 2, 3);
    try {
        print("" + predicateThrowArray.retainAll((value: number) => {
            if (value === 3) {
                throw "retainAll predicate error";
            }
            return value > 1;
        }));
    } catch (err) {
        print("retainAll predicate throw failed. err: " + err + ", code: " + err.code +
            ", array: " + predicateThrowArray);
    }
    const predicateConcurrentArray = new SendableArray<number>(1, 2);
    try {
        print("" + predicateConcurrentArray.retainAll((value: number) => {
            predicateConcurrentArray.includes(value);
            return true;
        }));
    } catch (err) {
        print("retainAll predicate concurrent failed. err: " + err + ", code: " + err.code +
            ", array: " + predicateConcurrentArray);
    }
    print("=== retainAll predicate repeat case ===");
    const repeatArray0 = new SendableArray<number>(1, 2, -3, 1);
    const repeatResult0 = repeatArray0.retainAll((value: number) => {
        return value > 0;
    });
    print("" + repeatResult0 + "," + repeatArray0);

    print("=== retainAll predicate receiver empty case ===");
    const emptyPredicateArray1 = new SendableArray<number>();
    let emptyPredicateCallCount = 0;
    const emptyPredicateResult1 = emptyPredicateArray1.retainAll((value: number) => {
        emptyPredicateCallCount++;
        return true;
    });
    print("" + emptyPredicateResult1 + "," + emptyPredicateArray1 + "," + emptyPredicateCallCount);

    print("=== retainAll receiver self test ===");
    const selfArray = new SendableArray<number>(1, 2);
    print("" + selfArray.retainAll(selfArray) + "," + selfArray);
    const subSharedReceiver3 = new SubSharedClass();
    subSharedReceiver3.push(1);
    subSharedReceiver3.push(2);
    subSharedReceiver3.push(3);
    print("" + subSharedReceiver3.retainAll(subSharedReceiver3) + "," + subSharedReceiver3);
    const retainAllConcurrentArray = new SendableArray<number>(1, 2);
    try {
        retainAllConcurrentArray.forEach((_value: number, _index: number, array: SendableArray<number>) => {
            array.retainAll(array);
        });
    } catch (err) {
        print("retainAll self concurrent failed. err: " + err + ", code: " + err.code +
            ", array: " + retainAllConcurrentArray);
    }

    print("=== retainAll exception case ===");
    try {
        print("" + array1.retainAll({0: 1, length: 1}));
    } catch (err) {
        print("retainAll arrayLike failed. err: " + err + ", code: " + err.code);
    }

    try {
        print("" + array1.retainAll());
    } catch (err) {
        print("retainAll undefined failed. err: " + err + ", code: " + err.code);
    }

    try {
        print("" + array1.retainAll(undefined));
    } catch (err) {
        print("retainAll undefined failed. err: " + err + ", code: " + err.code);
    }

    try {
        print("" + array1.retainAll(null));
    } catch (err) {
        print("retainAll null failed. err: " + err + ", code: " + err.code);
    }

    try {
        print("" + array1.retainAll(new Uint8Array([1, 2])));
    } catch (err) {
        print("retainAll typedArray failed. err: " + err + ", code: " + err.code);
    }

    try {
        print("" + array1.retainAll(new Set([1])));
    } catch (err) {
        print("retainAll set failed. err: " + err + ", code: " + err.code);
    }

    try {
        print("" + array1.retainAll(new SendableSet([1])));
    } catch (err) {
        print("retainAll sendableSet failed. err: " + err + ", code: " + err.code);
    }

    try {
        print("" + array1.retainAll({}));
    } catch (err) {
        print("retainAll object failed. err: " + err + ", code: " + err.code);
    }

    try {
        print("" + array1.retainAll(1));
    } catch (err) {
        print("retainAll primitive failed. err: " + err + ", code: " + err.code);
    }

    try {
        print("" + array1.retainAll(new Map([[1, 'one']]).entries()));
    } catch (err) {
        print("retainAll iterator failed. err: " + err + ", code: " + err.code);
    }

    const unboundRetainAll = array1.retainAll;
    const boundRetainAll = unboundRetainAll.bind(new SuperUnSharedClass(1));
    try {
        print("" + boundRetainAll([1]));
    } catch (err) {
        print("retainAll bind failed. err: " + err + ", code: " + err.code);
    }
    try {
        print("" + boundRetainAll((value: number) => value > 0));
    } catch (err) {
        print("retainAll bind failed. err: " + err + ", code: " + err.code);
    }

    const throwErrorElements = new Array<number>(5);
    throwErrorElements[0] = 1;
    // throwErrorElements[1] is a hole.
    throwErrorElements[2] = 3;
    throwErrorElements[3] = 4;
    Object.defineProperty(throwErrorElements, 4, {
        get() {
            throw "getter error";
        }
    });
    const elementsThrowArray = new SendableArray<number>(1, 2, 3);
    try {
        print("" + elementsThrowArray.retainAll(throwErrorElements) + "," +
            elementsThrowArray.length + "," + elementsThrowArray);
    } catch (err) {
        print("retainAll element throw failed. err: " + err + ", code: " + err.code +
            ", array: " + elementsThrowArray);
    }

    const throwAfterCompactElements = new Array<number>(3);
    throwAfterCompactElements[0] = 1;
    let throwAfterCompactGetterCount = 0;
    Object.defineProperty(throwAfterCompactElements, 1, {
        get() {
            throwAfterCompactGetterCount++;
            if (throwAfterCompactGetterCount >= 2) {
                throw "getter error after compact";
            }
            return 9;
        }
    });
    throwAfterCompactElements[2] = 4;
    const throwAfterCompactArray = new SendableArray<number>(2, 1, 3);
    try {
        print("" + throwAfterCompactArray.retainAll(throwAfterCompactElements) + "," +
            throwAfterCompactArray.length + "," + throwAfterCompactArray);
    } catch (err) {
        print("retainAll element throw after compact failed. err: " + err + ", code: " + err.code +
            ", array: " + throwAfterCompactArray);
    }
}

function index() {
    print("Start Test index")
    const array1 = new SendableArray<number>(5, 12, 8, 130, 44);
    const isLargeNumber = (element: number) => element > 13;
    print("" + array1.findIndex(isLargeNumber)); // 3

}

class IndexClass {
    idx: number;
    constructor() {
        this.idx = 1;
    }
}

function fill() {
    print("Start Test fill")
    const array1 = new SendableArray<number>(1, 2, 3, 4);
    array1.fill(0, 2, 4);
    print(array1); // [1, 2, 0, 0]

    array1.fill(5, 1);
    print(array1); // [1, 5, 5, 5]

    array1.fill(6);
    print(array1) // [6, 6, 6, 6]

    array1.fill(1, true);
    print(array1) // [6, 1, 1, 1]

    array1.fill(2, null);
    print(array1) // [2, 2, 2, 2]

    array1.fill(3, undefined);
    print(array1) // [3, 3, 3, 3]

    array1.fill(4, 1.5, 3.5);
    print(array1) // [3, 4, 4, 4]

    array1.fill(5, "1.0", "3.5");
    print(array1) // [3, 5, 5, 5]

    let superCs = new SuperClass();
    array1.fill(7, superCs, 4);
    print(array1) // [7, 7, 7, 7]

    let idxCs = new IndexClass();
    array1.fill(8, idxCs, 4);
    print(array1) // [8, 8, 8, 8]

    array1.fill(10, null, -1);
    print(array1) // [10, 10, 10, 8]

    array1.fill(11, -3, -1);
    print(array1) // [10, 11, 11, 11]

    array1.fill(11, -3, -0.1);
    print(array1) // [10, 11, 11, 11]

    array1.fill(12, 3, 1);
    print(array1) // [10, 11, 11, 11]
}

// remove
function pop() {
    print("Start Test pop")
    const sharedArray = new SendableArray<number>(5, 12, 8, 130, 44, 10, 20, 30, 100, 50, 80, 90, 150, 200);
    let sharedArray2 = sharedArray.concat(sharedArray).concat(sharedArray).concat(sharedArray).concat(sharedArray);
    print(sharedArray2.length);
    let len = sharedArray2.length;
    for (let idx = 0; idx < 10; ++idx) {
        sharedArray2.pop();
        print(sharedArray2)
    }
    print(sharedArray2.length);
}

// update
function randomUpdate() {
    print("Start Test randomUpdate")
    const sharedArray = new SendableArray<number>(5, 12, 8, 130, 44);
    sharedArray[1] = 30
    print(sharedArray[1]);
    try {
        sharedArray[null] = 30
    } catch (err) {
        print("add element by index access failed. err: " + err + ", code: " + err.code);
    }

    try {
        sharedArray[undefined] = 30
    } catch (err) {
        print("add element by index access failed. err: " + err + ", code: " + err.code);
    }

    try {
        sharedArray[2871622679] = 30
    } catch (err) {
        print("add element by index access failed. err: " + err + ", code: " + err.code);
    }
}

//  get
function randomGet() {
    print("Start Test randomGet")
    const sharedArray = new SendableArray<number>(5, 12, 8, 130, 44);
    sharedArray.at(0)
    print(sharedArray);
}

// add
function randomAdd() {
    print("Start Test randomAdd")
    const sharedArray = new SendableArray<number>(5, 12, 8);
    try {
        sharedArray[4000] = 7;
    } catch (err) {
        print("add element by index access failed. err: " + err + ", code: " + err.code);
    }
}

function create(): void {
    print("Start Test create")
    let arkTSTest: SendableArray<number> = new SendableArray<number>(5);
    let arkTSTest1: SendableArray<number> = new SendableArray<number>(1, 3, 5);
}

function from(): void {
    print("Start Test from")
    print(SendableArray.from<string>(['A', 'B', 'C']));
    try {
        print(SendableArray.from<string>(['E', , 'M', 'P', 'T', 'Y']));
    } catch (err) {
        print("Create from empty element list failed. err: " + err + ", code: " + err.code);
    }
    const source = new SendableArray<undefined>(undefined, undefined, 1);
    try {
        print('Create from sendable undefined element list success. arr: ' + SendableArray.from<string>(source));
    } catch (err) {
        print("Create from sendable undefined element list failed. err: " + err + ", code: " + err.code);
    }
    // trigger string cache
    SendableArray.from("hello");
    print(SendableArray.from("hello"));
    try {
        print(SendableArray.from.call({}, [1,2,3]))
    } catch (err) {
        print("Create from sendable list failed without constructor functions. err: " + err + ", code: " + err.code);
    }
  
    let sArr = SendableArray.from<string>(['A', 'B', 'C'], (str: string) => 'S' + str);
    print(sArr);
    let sArr1 = SendableArray.from<string>(sArr, (str: string) => 'S' + str);
    print(sArr1);
  
    try {
        let sArr2 = SendableArray.from<string>(sArr, (str: string) =>{
            return new SuperUnSharedClass(1);
        });
        print(sArr2);
    } catch (err) {
        print("Create from sendable array. err: " + err + ", code: " + err.code);
    }
  
    try {
        let sArr2 = SendableArray.from<SuperClass>(sArr, (str: string) =>{
            return new SuperClass(1);
        });
        sArr2.forEach((element: SubClass) => print(element.num)); // 5, 8, 44
    } catch (err) {
        print("Create from sendable array. err: " + err + ", code: " + err.code);
    }
  
    let normalArr = new Array<number>(3,2,1,5);
    let sArr3 = SendableArray.from<number>(normalArr, (num: number) =>{
        normalArr.pop();
        return num += 1;
    });
    print(sArr3)

    let normalArr1 = new Array<number>(3,2,1,5);
    let sArr4 = SendableArray.from<number>(normalArr1, (num: number) =>{
        if (num < 3) {
          normalArr1.push(num + 1);
        }  
  
        return num += 1;
    });
    print(sArr4)

    let normalArr2 = new Array<string>("abcd","bcde","cdef","cfgh");
    let sArr5 = SendableArray.from<number>(normalArr2, (str: string) =>{
        if (str.length <= 4) {
            normalArr2.push(str + "cde");
        }  
  
        return str + "cde";
    });
    print(sArr5)

    let obj = {
        0:1,
        1:3,
        2:5,
        length:3
    };
    let sArr6 = SendableArray.from<number>(obj, (value: number) =>{
        if (value < 4) {
            obj[obj.length] = value + 10;
            obj.length += 1;
        }  
  
        return value;
    });
    print(sArr6)
}

function fromTemplate(): void {
  print('Start Test fromTemplate');
  let artTSTest1: SendableArray<string> = SendableArray.from<Number, string>([1, 2, 3], (x: number) => '' + x);
  print('artTSTest1: ' + artTSTest1);
  let arkTSTest2: SendableArray<string> = SendableArray.from<Number, string>([1, 2, 3], (item: number) => '' + item); // ["1", "Key", "3"]
  print('arkTSTest2: ' + arkTSTest2);
}

function length(): void {
    print("Start Test length")
    let array: SendableArray<number> = new SendableArray<number>(1, 3, 5);
    print("Array length: " + array.length);
    array.length = 50;
    print("Array length after changed: " + array.length);
}

function push(): void {
    print("Start Test push")
    let array: SendableArray<number> = new SendableArray<number>(1, 3, 5);
    array.push(2, 4, 6);
    print("Elements pushed: " + array);
    let array1 = new SendableArray<number>();
    array1.push(1);
    array1.push(2);
    array1.push(3);
    array1.push(7, 8, 9);
    print("Elements pushed: " + array1);
}

function concat(): void {
    print("Start Test concat")
    let array: SendableArray<number> = new SendableArray<number>(1, 3, 5);
    let arkTSToAppend: SendableArray<number> = new SendableArray<number>(2, 4, 6);
    let arkTSToAppend1: SendableArray<number> = new SendableArray<number>(100, 101, 102);

    print(array.concat(arkTSToAppend)); // [1, 3, 5, 2, 4, 6]
    print(array.concat(arkTSToAppend, arkTSToAppend1));
    print(array.concat(200));
    print(array.concat(201, 202));
    let arr: SendableArray<number> = array.concat(null);
    print(arr);
    print(arr[3]);
    print(arr.length);
    let arr1: SendableArray<number> = array.concat(undefined);
    print(arr1);
    print(arr1[3]);
    print(arr1.length);

    let nsArr = [1, , 4];
    let arr2: SendableArray<number> = array.concat(1, nsArr[1], 5);
    print(arr2);
    print(arr2.length);

    let arr3: SendableArray<number> = array.concat(1, arr1, 5, nsArr[1]);
    print(arr3);
    print(arr3.length);
}

function join(): void {
    print("Start Test join")
    const elements = new SendableArray<string>('Fire', 'Air', 'Water');
    print(elements.join());
    print(elements.join(''));
    print(elements.join('-'));
    print(elements.join(null));
    print(elements.join(undefined));

    const elements1 = new SendableArray<string>("123", "3445", "789");
    const elements2 = new SendableArray<SendableArray>();
    elements2.push(elements1);
    elements2.push(elements2);
  
    print(elements2.join());
}

function shift() {
    print("Start Test shift")
    const array1 = new SendableArray<number>(2, 4, 6);
    print(array1.shift());
    print(array1.length);

    const emptyArray = new SendableArray<number>();
    print(emptyArray.shift());

    const array2 = new SendableArray<number>(2, 4, 6, 100, 50, 60, 70);
    let array3 = array2.concat(array2).concat(array2).concat(array2).concat(array2).concat(array2).concat(array2);
    print(array3);
    print(array3.length);
    let len = array3.length;
    for (let idx = 0; idx < 10; ++idx) {
        array3.shift();
        print(array3);
    }
    print(array3.length)
}

function unshift() {
    print("Start Test unshift")
    const array = SendableArray.from<number>([1, 2, 3]);
    print(array);
    print(array.length);
    print(array.unshift(4, 5));
    print(array);
    print(array.length);

    let arr2 = array.concat(array).concat(array).concat(array).concat(array).concat(array).concat(array);
    print(arr2);
    print(arr2.length);
    let arr3 = arr2.concat(arr2);
    print(arr3);
    print(arr3.length);
    print(arr2.unshift(arr3));
    print(arr2);
    print(arr2.length);
    
}

function slice() {
    print("Start Test slice")
    const animals = new SendableArray<string>('ant', 'bison', 'camel', 'duck', 'elephant');
    print(animals.slice());
    print(animals.slice(2));
    print(animals.slice(2, 4));
    try {
        let a1 = animals.slice(1.5, 4);
        print("slice(1.5, 4) element success");
        print(a1);
    } catch (err) {
        print("slice element failed. err: " + err + ", code: " + err.code);
    }

    try {
        let a2 = animals.slice(8, 4);
        print("slice(8, 4) element success");
    } catch (err) {
        print("slice element failed. err: " + err + ", code: " + err.code);
    }

    try {
        let a3 = animals.slice(8, 100);
        print("slice(8, 100) element success");
    } catch (err) {
        print("slice element failed. err: " + err + ", code: " + err.code);
    }

    try {
        print(animals.slice(null));
    } catch (err) {
        print("slice element failed. err: " + err + ", code: " + err.code);
    }

    try {
        print(animals.slice(undefined));
    } catch (err) {
        print("slice element failed. err: " + err + ", code: " + err.code);
    }
}

function sort() {
    print("Start Test sort")
    const months = new SendableArray<string>('March', 'Jan', 'Feb', 'Dec');
    print(months.sort());

    const array1 = [1, 30, 4, 21, 10000];
    print(array1.sort());

    array1.sort((a: number, b: number) => a - b);
}

function indexOf() {
    print("Start Test indexOf")
    const beasts = new SendableArray<string>('ant', 'bison', 'camel', 'duck', 'bison');
    print(beasts.indexOf('bison')); // Expected: 1
    print(beasts.indexOf('bison', 2)) // Expected: 4
    print(beasts.indexOf('giraffe')) // Expected： -1
}

function forEach() {
  print('Start Test forEach');
  const array = new SendableArray<string>('a', 'b', 'c');
  array.forEach((element: string) => print(element)); // a <br/> b <br/>  c

  array.forEach((element: string, index: number, array: SendableArray<string>) =>
    print(`a[${index}] = ${element}, ${array[index]}`),
  );
}

function map() {
    print("Start Test map")
    const array = new SendableArray<number>(1, 4, 9, 16);
    print(array.map<string>((x: number) => x + x));
}

function filter() {
    print("Start Test filter")
    const words = new SendableArray<string>('spray', 'elite', 'exuberant', 'destruction', 'present');
    print(words.filter((word: string) => word.length > 6))
    const array2 = new SendableArray<SuperClass>(
      new SubClass(5),
      new SuperClass(12),
      new SubClass(8),
      new SuperClass(130),
      new SubClass(44),
    );
    const result = array2.filter<SubClass>((value: SuperClass, index: number, obj: Array<SuperClass>) => value instanceof SubClass);
    result.forEach((element: SubClass) => print(element.num)); // 5, 8, 44

    const words1 = new SendableArray<string>('spray', 'elite', 'exuberant', 'destruction', 'present');
    let words2 = words1.concat(words).concat(words).concat(words).concat(words).concat(words).concat(words).concat(words);
    print(words2)
    print(words2.filter((word: string) => word.length > 10))
}

function reduce() {
    print("Start Test reduce")
    const array = new SendableArray<number>(1, 2, 3, 4);
    print(array.reduce((acc: number, currValue: number) => acc + currValue)); // 10

    print(array.reduce((acc: number, currValue: number) => acc + currValue, 10)); // 20

    print(array.reduce<string>((acc: number, currValue: number) => "" + acc + " " + currValue, "10")); // 10, 1, 2, 3, 4
}

function f0() {
    const o1 = {
    };

    return o1;
}

const v2 = f0();
class C3 {
    constructor(a5,a6) {
        const v9 = new SendableArray();
        v9.splice(0,0, v2);
    }
}

function splice() {
    print("Start Test splice")
    const array = new SendableArray<string>('Jan', 'March', 'April', 'June');
    print(array.splice());
    print(array);
    array.splice(1, 0, 'Feb', 'Oct');
    print(array); // "Jan", "Feb", "Oct", "March", "April", "June"
    const removeArray = array.splice(4, 2, 'May');
    print(array); // "Jan", "Feb", "Oct", "March", "May"
    print(removeArray); // "April", "June"
    const removeArray1 = array.splice(2, 3);
    print(array); // "Jan", "Feb"
    print(removeArray1); // "Oct", "March", "May"

    const array2 = new SendableArray<SubClass>(
        new SubClass(5),
        new SubClass(32),
        new SubClass(8),
        new SubClass(130),
        new SubClass(44),
    );

    try {
        array2.splice(0, 0, new SuperUnSharedClass(48));
        print("Add one element by splice api.");
    } catch (err) {
        print("Add one element by splice api failed. err: " + err + ", code: " + err.code);
    }

    try {
        new C3();
        print("Add one element by splice api.");
    } catch (err) {
        print("Add one element by splice api failed. err: " + err + ", code: " + err.code);
    }
}

function staticCreate() {
    print("Start Test staticCreate")
    const array = SendableArray.create<number>(10, 5);
    print(array);
    try {
        const array = SendableArray.create<number>(5);
        print("Create with without initialValue success.");
    } catch (err) {
        print("Create with without initialValue failed. err: " + err + ", code: " + err.code);
    }
    try {
        const array = SendableArray.create<number>(-1, 5);
        print("Create with negative length success.");
    } catch (err) {
        print("Create with negative length failed. err: " + err + ", code: " + err.code);
    }
    try {
        const array = SendableArray.create<number>(13107200, 1); // 13107200: 12.5MB
        print("Create huge sendableArrayWith initialValue success.");
    } catch (err) {
        print("Create huge sendableArrayWith initialValue failed. err: " + err + ", code: " + err.code);
    }
    try {
        const array = SendableArray.create<number>(0x100000000, 5);
        print("Create with exceed max length success.");
    } catch (err) {
        print("Create with exceed max length failed. err: " + err + ", code: " + err.code);
    }
}

function readonlyLength() {
    print("Start Test readonlyLength")
    const array = SendableArray.create<number>(10, 5);
    print(array.length);
    array.length = 0;
    print(array.length);
}

function shrinkTo() {
    print("Start Test shrinkTo")
    const array = new SendableArray<number>(5, 5, 5, 5, 5, 5, 5, 5, 5, 5);
    print(array.length);
    array.shrinkTo(array.length);
    print("Shrink to array.length: " + array);
    array.shrinkTo(array.length + 1);
    print("Shrink to array.length + 1: " + array);
    try {
        array.shrinkTo(-1);
        print("Shrink to -1 success");
    } catch (err) {
        print("Shrink to -1 fail. err: " + err + ", code: " + err.code);
    }
    try {
        array.shrinkTo(0x100000000);
        print("Shrink to invalid 0x100000000 success");
    } catch (err) {
        print("Shrink to invalid 0x100000000 fail. err: " + err + ", code: " + err.code);
    }    
    array.shrinkTo(1);
    print(array.length);
    print(array);

}

function extendTo() {
    print("Start Test growTo")
    const array = SendableArray.create<number>(5, 5);
    print(array.length);
    array.extendTo(array.length, 0);
    print("ExtendTo to array.length: " + array);
    array.extendTo(array.length - 1, 0);
    print("ExtendTo to array.length - 1: " + array);
    array.extendTo(0, 0);
    print("ExtendTo to 0: " + array);
    try {
        array.extendTo(-1, 0);
        print("ExtendTo to -1 success.");
    } catch (err) {
        print("ExtendTo to -1 fail. err: " + err + ", code: " + err.code);
    }
    try {
        array.extendTo(0x100000000, 0);
        print("ExtendTo to invalid 0x100000000 success.");
    } catch (err) {
        print("ExtendTo to invalid 0x100000000 fail. err: " + err + ", code: " + err.code);
    }
    try {
        array.extendTo(8);
        print("ExtendTo to 8 without initValue success.");
    } catch (err) {
        print("ExtendTo to 8 without initValue fail. err: " + err + ", code: " + err.code);
    }
    array.extendTo(8, 11);
    print(array.length);
    print(array);
}

function indexAccess() {
    print("Start Test indexAccess")
    const array = new SendableArray<number>(1, 3, 5, 7);
    print("element1: " + array[1]);
    array[1] = 10
    print("element1 assigned to 10: " + array[1]);
    try {
        array[10]
        print("Index access read out of range success.");
    } catch (err) {
        print("Index access read out of range failed. err: " + err + ", code: " + err.code);
    }
    try {
        array[100] = 10
        print("Index access write out of range success.");
    } catch (err) {
        print("Index access write out of range failed. err: " + err + ", code: " + err.code);
    }
    try {
        array.forEach((key: number, _: number, array: SendableArray) => {
          array[key + array.length];
        });
    } catch (err) {
        print("read element while iterate array fail. err: " + err + ", errCode: " + err.code);
    }
    try {
        array.forEach((key: number, _: number, array: SendableArray) => {
          array[key + array.length] = 100;
        });
    } catch (err) {
        print("write element while iterate array fail. err: " + err + ", errCode: " + err.code);
    }
}

function indexStringAccess() {
    print("Start Test indexStringAccess")
    const array = new SendableArray<number>(1, 3, 5, 7);
    print("String index element1: " + array["" + 1]);
    array["" + 1] = 10
    print("String index element1 assigned to 10: " + array["" + 1]);
    try {
        array["" + 10]
        print("String Index access read out of range success.");
    } catch (err) {
        print("String Index access read out of range failed. err: " + err + ", code: " + err.code);
    }
    try {
        array["" + 100] = 10
        print("String Index access write out of range success.");
    } catch (err) {
        print("String Index access write out of range failed. err: " + err + ", code: " + err.code);
    }
    try {
        array.forEach((key: number, _: number, array: SendableArray) => {
          array['' + key + array.length];
        });
    } catch (err) {
        print("String index read element while iterate array fail. err: " + err + ", errCode: " + err.code);
    }
    try {
        array.forEach((key: number, _: number, array: SendableArray) => {
          array['' + key + array.length] = 100;
        });
    } catch (err) {
        print("String index write element while iterate array fail. err: " + err + ", errCode: " + err.code);
    }
}

function testForIC(index: number) {
    const array = new SendableArray<number>(1, 3, 5, 7);
    try {
        const element = array[index < 80 ? 1 : 10];
        if (index == 1) {
            print("[IC] Index access read in range success. array: " + element);
        }
    } catch (err) {
        if (index == 99) {
            print("[IC] Index access read out of range failed. err: " + err + ", code: " + err.code);
        }
    }
    try {
        array[index < 80 ? 1 : 100] = 10
        if (index == 1) {
            print("[IC] Index access write in range success.");
        }
    } catch (err) {
        if (index == 99) {
            print("[IC] Index access write out of range failed. err: " + err + ", code: " + err.code);
        }
    }
    try {
        array.length = index < 80 ? 1 : 100;
        if (index == 1) {
            print("[IC] assign readonly length no error.");
        }
    } catch (err) {
        if (index == 99) {
            print("[IC] assign readonly length fail. err: " + err + ", code: " + err.code);
        }
    }
}

function testStringForIC(index: number) {
    const array = new SendableArray<number>(1, 3, 5, 7);
    try {
        const element = array["" + index < 80 ? 1 : 10];
        if (index == 1) {
            print("[IC] String Index access read in range success. array: " + element);
        }
    } catch (err) {
        if (index == 99) {
            print("[IC] String Index access read out of range failed. err: " + err + ", code: " + err.code);
        }
    }
    try {
        array["" + (index < 80 ? 1 : 100)] = 10
        if (index == 1) {
            print("[IC] String Index access write in range success.");
        }
    } catch (err) {
        if (index == 99) {
            print("[IC] String Index access write out of range failed. err: " + err + ", code: " + err.code);
        }
    }
}

function frozenTest(array: SendableArray) {
  try {
    array.notExistProp = 1;
  } catch (err) {
    print('Add prop to array failed. err: ' + err);
  }
  try {
    Object.defineProperty(array, 'defineNotExistProp', { value: 321, writable: false });
  } catch (err) {
    print('defineNotExistProp to array failed. err: ' + err);
  }
  try {
    array.at = 1;
  } catch (err) {
    print('Update function [at] failed. err: ' + err);
  }
  try {
    Object.defineProperty(array, 'at', { value: 321, writable: false });
  } catch (err) {
    print('Update function [at] by defineProperty failed. err: ' + err);
  }
  array.push(111);
}

function arrayFrozenTest() {
    print("Start Test arrayFrozenTest")
    let arr1 = new SendableArray<string>('ARK');
    print("arrayFrozenTest [new] single string. arr: " + arr1);
    frozenTest(arr1);
    arr1 = new SendableArray<string>('A', 'R', 'K');
    print("arrayFrozenTest [new]. arr: " + arr1);
    frozenTest(arr1);
    arr1 = SendableArray.from<string>(['A', 'R', 'K']);
    print("arrayFrozenTest static [from]. arr: " + arr1);
    frozenTest(arr1);
    arr1 = SendableArray.create<string>(3, 'A');
    print("arrayFrozenTest static [create]. arr: " + arr1);
    frozenTest(arr1);
}

function sharedArrayFrozenTest() {
    print("Start Test sharedArrayFrozenTest")
    let arr1 = new SubSharedClass();
    arr1.push("A");
    arr1.push("R");
    arr1.push("K");
    print("sharedArrayFrozenTest [new]. arr: " + arr1);
    frozenTest(arr1);
}

function increaseArray() {
    print("Start Test extendSharedTest")
    let sub = new SubSharedClass();
    for (let idx: number = 0; idx < 1200; idx++) {
        sub.push(idx + 10);
    }
    print("Push: " + sub);
}

function arrayFromSet(){
  print('Start Test arrayFromSet');
  const set = new Set(['foo', 'bar', 'baz', 'foo']);
  const sharedSet = new SendableSet(['foo', 'bar', 'baz', 'foo']);
  print('Create from normal set: ' + SendableArray.from(set));
  print('Create from shared set: ' + SendableArray.from(set));
}

function arrayFromNormalMap() {
    print("Start Test arrayFromNormalMap")
    const map = new Map([
        [1, 2],
        [2, 4],
        [4, 8],
      ]);
      Array.from(map);
      // [[1, 2], [2, 4], [4, 8]]
      
      const mapper = new Map([
        ["1", "a"],
        ["2", "b"],
      ]);
      Array.from(mapper.values());
      // ['a', 'b'];
      
      Array.from(mapper.keys());
      // ['1', '2'];
}

function arrayFromSendableMap() {
  print('Start test arrayFromSendableMap');
  const map = new SendableMap([
    [1, 2],
    [2, 4],
    [4, 8],
  ]);
  try {
    print('create from sharedMap: ' + SendableArray.from(map));
  } catch (err) {
    print('create from sharedMap with non-sendable array failed. err: ' + err + ', code: ' + err.code);
  }
  // [[1, 2], [2, 4], [4, 8]]

  const mapper = new SendableMap([SendableArray.from(['1', 'a']), SendableArray.from(['2', 'b'])]);
  print('create from sharedMapper.values(): ' + SendableArray.from(mapper.values()));
  // ['a', 'b'];

  print('create from sharedMapper.values(): ' + SendableArray.from(mapper.keys()));
  // ['1', '2'];
}

function arrayFromNotArray() {
    print("Start test arrayFromNotArray")
    function NotArray(len: number) {
        print("NotArray called with length", len);
    }

    try {
        print('Create array from notArray: ' + SendableArray.from.call(NotArray, new Set(['foo', 'bar', 'baz'])));
    } catch (err) {
        print("Create array from notArray failed. err: " + err + ", code: " + err.code);
    }
}

function derivedSlice() {
    print("Start Test derivedSlice")
    let animals = new SubSharedClass();
    animals.push('ant');
    animals.push('bison');
    animals.push('camel');
    animals.push('duck');
    animals.push('elephant');
    print("instanceOf slice result: " + (animals.slice() instanceof SubSharedClass));
}

function derivedSort() {
    print("Start Test derivedSort")
    let months = new SubSharedClass();
    months.push('March')
    months.push('Jan')
    months.push('Feb')
    months.push('Dec')

    const sortedMonth = months.sort();
    print("instanceOf derived sort result: " + (sortedMonth instanceof SubSharedClass));
}

function derivedForEach() {
  print('Start Test derivedForEach');
  let array = new SubSharedClass();
  array.push('March');
  array.push('Jan');
  array.push('Feb');
  array.push('Dec');
  array.forEach((element: string, index: number, array: SendableArray<string>) =>
    print(`a[${index}] = ${element}, ${array instanceof SubSharedClass}`),
  );
}

function derivedMap() {
    print("Start derivedMap")
    let array = new SubSharedClass();
    array.push(1);
    array.push(4);
    array.push(9);
    array.push(16);
    print("instanceOf derived map result: " + (array.map<string>((x: number) => x + x + "") instanceof SubSharedClass));
}

function derivedFill() {
    print("Start Test derivedFill")
    let array = new SubSharedClass();
    array.push(1);
    array.push(2);
    array.push(3);
    array.push(4);
    const filledArray = array.fill(0, 2, 4);
    print(array); // [1, 2, 0, 0]
    print("instanceOf derived fill result: " + (filledArray instanceof SubSharedClass));
}

function readOutOfRange() {
    print("Start Test array read out of range")
    const array = new SendableArray<number>(1, 3, 5, 7);
    print("array[0]: " + array[0]);
    try {
        let value = array[9];
        print("read out of range success " + value);
    } catch (err) {
        print("read out of range failed. err: " + err + ", code: " + err.code);
    }

    try {
        let value = array['0'];
        print("read out of range success " + value);
    } catch (err) {
        print("read out of range failed. err: " + err + ", code: " + err.code);
    }

    try {
        let value = array[0.0];
        print("read out of range success " + value);
    } catch (err) {
        print("read out of range failed. err: " + err + ", code: " + err.code);
    }

    try {
        let value = array[1.5]
        print("read out of range success " + value);
    } catch (err) {
        print("read out of range failed. err: " + err + ", code: " + err.code);
    }

    try {
        let value = array[undefined]
        print("read out of range success " + value);
    } catch (err) {
        print("read out of range failed. err: " + err + ", code: " + err.code);
    }

    try {
        let value = array[null]
        print("read out of range success " + value);
    } catch (err) {
        print("read out of range failed. err: " + err + ", code: " + err.code);
    }

    try {
        let value = array[Symbol.toStringTag]
        print("read out of range success " + value);
    } catch (err) {
        print("read out of range failed. err: " + err + ", code: " + err.code);
    }

    try {
        let value = array[false]
        print("read out of range success " + value);
    } catch (err) {
        print("read out of range failed. err: " + err + ", code: " + err.code);
    }

    try {
        let value = array[true]
        print("read out of range success " + value);
    } catch (err) {
        print("read out of range failed. err: " + err + ", code: " + err.code);
    }
}

function forOf() {
    print("Start Test array for of")
    const array = new SendableArray<number>(1, 3, 5, 7);
    for(const num of array){
        print(num);
    }
}

function sharedArrayConstructorTest(){
    let from_arr  = [1,2,3];
    let s_arr = new SendableArray<number>(...from_arr); // output [1,2,3]
    print(("SendableArray ...from_arr: " + s_arr));
    let s_arr1 = new SendableArray<number>(0, ...from_arr); // output [1,2,3]
    print(("SendableArray ...from_arr1: " + s_arr1));
    try {
        print("Create from SendableArray with non-sendable array error: " + new SendableArray(from_arr));
    } catch (err) {
        print("Create from SendableArray with non-sendable array error failed. err: " + err + ", code: " + err.code);
    }
}

function fromArrayConstructorTest(): void {
    print("Start Test fromArrayConstructorTest")
    const array1 = new SendableArray<string>('a', 'b', 'c');
    const iterator = array1.values();
    print(SendableArray.from<string>(iterator));
}

function DefinePropertyTest() {
    print("Start Test DefinePropertyTest")
    let array = new SendableArray<string>('ARK');
    try {
        Object.defineProperty(array, '0', {writable: true, configurable: true, enumerable: true, value: "321"});
        print('defineProperty to array success');
    } catch (err) {
        print('defineProperty to array failed. err: ' + err);
    }

    try {
        Object.defineProperty(array, '1200', {writable: true, configurable: true, enumerable: true, value: "321"});
        print('defineProperty to array success');
    } catch (err) {
        print('defineProperty to array failed. err: ' + err);
    }

    try {
        Object.defineProperty(array, 0, {writable: true, configurable: true, enumerable: true, value: "321"});
        print('defineProperty to array success');
    } catch (err) {
        print('defineProperty to array failed. err: ' + err);
    }

    try {
        Object.defineProperty(array, 1200, {writable: true, configurable: true, enumerable: true, value: "321"});
        print('defineProperty to array success');
    } catch (err) {
        print('defineProperty to array failed. err: ' + err);
    }

    try {
        Object.defineProperty(array, 2871622679, {writable: true, configurable: true, enumerable: true, value: "321"});
        print('defineProperty to array success');
    } catch (err) {
        print('defineProperty to array failed. err: ' + err);
    }
    try {
        Object.defineProperty(array, 0.0, {writable: true, configurable: true, enumerable: true, value: "321"});
        print('defineProperty to array success ' + array[0.0]);
    } catch (err) {
        print("defineProperty to array failed. err: " + err + ", code: " + err.code);
    }

    try {
        Object.defineProperty(array, 1.5, {writable: true, configurable: true, enumerable: true, value: "321"});
        print('defineProperty to array success ' + array[1.5]);
    } catch (err) {
        print("defineProperty to array failed. err: " + err + ", code: " + err.code);
    }

    try {
        Object.defineProperty(array, undefined, {writable: true, configurable: true, enumerable: true, value: "321"});
        print("defineProperty to array success " + array[undefined]);
    } catch (err) {
        print("defineProperty to array failed. err: " + err + ", code: " + err.code);
    }

    try {
        Object.defineProperty(array, null, {writable: true, configurable: true, enumerable: true, value: "321"});
        print("defineProperty to array success " + array[null]);
    } catch (err) {
        print("defineProperty to array failed. err: " + err + ", code: " + err.code);
    }

    try {
        Object.defineProperty(array, Symbol.toStringTag, {writable: true, configurable: true, enumerable: true, value: "321"});
        print("defineProperty to array success " + array[Symbol.toStringTag]);
    } catch (err) {
        print("defineProperty to array failed. err: " + err + ", code: " + err.code);
    }

    try {
        Object.defineProperty(array, true, {writable: true, configurable: true, enumerable: true, value: "321"});
        print("defineProperty to array success " + array[null]);
    } catch (err) {
        print("defineProperty to array failed. err: " + err + ", code: " + err.code);
    }

    try {
        Object.defineProperty(array, false, {writable: true, configurable: true, enumerable: true, value: "321"});
        print("defineProperty to array success " + array[Symbol.toStringTag]);
    } catch (err) {
        print("defineProperty to array failed. err: " + err + ", code: " + err.code);
    }
}

function isEven(num) {
    return num % 2 === 0;
}

function SomeTest(): void {
    print("Start Test SomeTest")
    const numbers = new SendableArray<number>(1, 2, 3, 4, 5);

    const hasEvenNumber = numbers.some(isEven); // 1: Whether there are even numbers in the array
    print(hasEvenNumber); // should be true

    const hasNegativeNumber = numbers.some(num => num < 0); // 2:  Whether there are negative numbers in the array
    print(hasNegativeNumber); // should be false

    let nsArr = [1, , 3];
    const numbers1 = new SendableArray<number>(nsArr[1], 2, 3, 4, 5);
    print(numbers1.some(num => num < 0)); // should be false
    print(numbers1.some(num => num < 0 || num === undefined)); // should be true
    print(numbers1.some(num => num)); // should be true

    const numbers2 = new SendableArray<number>(null, undefined, 3, 4, 5);
    print(numbers2.some(num => num < 0)); // should be false
    print(numbers2.some(num => num < 0 || num === undefined)); // should be true
    print(numbers2.some(num => num)); // should be true
}

function EveryTest(): void {
    print("Start Test EveryTest")
    const numbers = new SendableArray<number>(1, 2, 3, 4, 5);

    const allPositive = numbers.every((num) => num > 0); // Check that all elements in the array are greater than 0
    print(allPositive); // should be true

    const allEven = numbers.every((num) => num % 2 === 0); // Check if all the elements in the array are even
    print(allEven); // should be false

    let nsArr = [1, , 3, 5];
    const numbers1 = new SendableArray<number>(nsArr[0], nsArr[1], undefined, 4, 5);
    const allPositive1 = numbers1.every((num) => num > 0);
    print(allPositive1); // should be false

    const allPositive2 = numbers1.every((num) => num > 0 || num == undefined);
    print(allPositive2); // should be true
}

function isArrayTest() {
  // print true
  print("Start Test isArrayTest")
  print(SendableArray.isArray(new SendableArray()));
  print(SendableArray.isArray(new SendableArray('a', 'b', 'c', 'd')));
  print(SendableArray.isArray(new SendableArray(3)));
  print(SendableArray.isArray(SendableArray.prototype));

  // print false
  print(SendableArray.isArray([]));
  print(SendableArray.isArray([1]));
  print(SendableArray.isArray());
  print(SendableArray.isArray({}));
  print(SendableArray.isArray(null));
  print(SendableArray.isArray(undefined));
  print(SendableArray.isArray(17));
  print(SendableArray.isArray('SendableArray'));
  print(SendableArray.isArray(true));
  print(SendableArray.isArray(false));
  print(SendableArray.isArray(new SendableUint8Array(32)));
}

function lastIndexOfTest() {
  print("Start Test lastIndexOf")
  let arr = SendableArray.from([1, 2, 3, 4, 2, 5]);
  print(arr.lastIndexOf(2));

  print(arr.lastIndexOf(1));
  print(arr.lastIndexOf(5));
  print(arr.lastIndexOf(6));

  let emptyArr = SendableArray.from([]);
  print(emptyArr.lastIndexOf(1));

  let arrWithNaN = SendableArray.from([1, 2, NaN, 4, NaN]);
  print(arrWithNaN.lastIndexOf(NaN));

  let arrWithUndefined = SendableArray.from([1, 2, undefined, 4]);
  print(arrWithUndefined.lastIndexOf(undefined));
}

function ofTest() {
    print("Start Test ofTest")
    let arr = SendableArray.of(1, 2, 3, 4, 2, 5);
    print(arr);

    arr = SendableArray.of();
    print(arr);
  
    arr = SendableArray.of.call({}, 1,2,3,4,5);
    print(arr);

    try {
        arr = SendableArray.of([1,2,3,4,5]);
    } catch (err) {
        print("Create SendableArray failed. err: " + err + ", code: " + err.code);
    }

    try {
        arr = SendableArray.of.call(Array, 1,2,3,4,5);
    } catch (err) {
        print("Create SendableArray failed. err: " + err + ", code: " + err.code);
    }
}

function copyWithinTest() {
    print("Start Test copyWithin")
    let arr = new SendableArray(1, 2, 3, 4, 2, 5);
    print(arr);

    try {
        arr.copyWithin();
    } catch (err) {
        print("copyWithin SendableArray failed. err: " + err + ", code: " + err.code);
    }

    arr.copyWithin(1);
    print(arr);
  
    arr.copyWithin(2, 3);
    print(arr);

    arr.copyWithin(3, -4, -2);
    print(arr);

    arr.copyWithin(2, -3, -4);
    print(arr);

    arr.copyWithin(4, 2, 1);
    print(arr);
}

function findLast() {
    print("Start Test findLast")
    const array1 = new SendableArray<number>(5, 12, 8, 130, 44);

    const found = array1.findLast((element: number) => element > 100);
    print("" + found); // 130

    const found2 = array1.findLast((element: number) => element > 200);
    print("" + found2); // undefined

    try {
        const found3 = array1.findLast(130);
        print("" + found3);
    } catch (err) {
        print("findLast failed. err: " + err + ", code: " + err.code);
    }

    const array2 = new SendableArray<SuperClass>(
      new SubClass(5),
      new SubClass(32),
      new SubClass(8),
      new SubClass(130),
      new SubClass(44),
    );
    const result: SubClass | undefined = array2.findLast<SubClass>(
      (value: SuperClass, index: number, obj: SendableArray<SuperClass>) => value instanceof SubClass,
    );
    print(result.strProp); // 44
}

function findLastIndex() {
    print("Start Test findLastIndex")
    const array1 = new SendableArray<number>(5, 12, 8, 130, 44);

    try {
        const found3 = array1.findLastIndex(130);
        print("" + found3);
    } catch (err) {
        print("findLastIndex failed. err: " + err + ", code: " + err.code);
    }

    const found = array1.findLastIndex((element: number) => element > 100);
    print("find index1: " + found); // 3

    const found2 = array1.findLastIndex((element: number) => element > 200);
    print("find index2: " + found2); // -1

    const array2 = new SendableArray<SuperClass>(
      new SubClass(5),
      new SubClass(32),
      new SubClass(8),
      new SubClass(130),
      new SubClass(44),
    );
    const result: SubClass | undefined = array2.findLastIndex<SubClass>(
      (value: SuperClass, index: number, obj: SendableArray<SuperClass>) => value instanceof SubClass,
    );
    print(result); // index is 4
}

function reduceRight() {
    print("Start Test reduceRight")
    const array = new SendableArray<number>(1, 2, 3, 4);
    print(array.reduceRight((acc: number, currValue: number) => acc + currValue)); // 10

    print(array.reduceRight((acc: number, currValue: number) => acc + currValue, 10)); // 20

    print(array.reduceRight<string>((acc: number, currValue: number) => "" + acc + " " + currValue, "10")); // 10, 4, 3, 2, 1

    const array1 = new SendableArray<number>(1, 2, 3, 4);
    print(array1.reduceRight((acc: number, currValue: number) => acc + currValue, undefined));

    const array2 = new SendableArray<number>();
    print(array2.reduceRight((acc: number, currValue: number) => acc + currValue, 1));
    print(array2.reduceRight((acc: number, currValue: number) => acc + currValue, undefined));

    try {
       print(array2.reduceRight((acc: number, currValue: number) => acc + currValue));
    } catch (err) {
       print("reduceRight failed. err: " + err + ", code: " + err.code);
    }

    try {
        print(array2.reduceRight(1, 1));
    } catch (err) {
        print("reduceRight failed. err: " + err + ", code: " + err.code);
    }
}

function reverse() {
    print("Start Test reverse")
    const array1 = new SendableArray<number>(1, 2, 3, 4);
    print(array1.reverse());
	print(array1);

	const array2 = new SendableArray<number>("one", "two", "three");
    print(array2.reverse());
	print(array2);
}

function toStringTest() {
    print("Start Test toString")
    const array1 = new SendableArray<number>(1, 2, 3, 4);
    print(array1.toString());

	const array2 = new SendableArray<string>("one", "two", "three");
    print(array2.toString());

	const array3 = new SendableArray<number>(null, undefined, 3, 4, 5);
	print(array3.toString());
}

function toLocaleStringTest() {
    print("Start Test toLocaleString")
    const array1 = new SendableArray<number>(1, 2, 3, 4);
    print(array1.toLocaleString());

	const array2 = new SendableArray<string>("one", "two", "three");
    print(array2.toLocaleString());

	const array3 = new SendableArray<number>(null, undefined, 3, 4, 5);
	print(array3.toLocaleString());

    const array4 = new SendableArray(1000, 2000, 3000, 4000, 5000);
    print(array4.toLocaleString('de-DE'));
    print(array4.toLocaleString('fr-FR'));

    const array5 = new SendableArray<number>(123456.789, 2000.00);
    print(array5.toLocaleString('en-US', {style: 'currency', currency: 'USD'}));
    print(array5.toLocaleString('de-DE', {style: 'currency', currency: 'USD'}));

    let array6 = new SendableArray<number>(123456.789, 2000.00);
    let array7 = new SendableArray<number>(3, 4, 5);
    array6.push(array7);
    print(array6.toLocaleString('de-DE', {style: 'currency', currency: 'USD'}));
}

class SubSendableArray<T> extends SendableArray<T> {
    desc: string = "I'am SubSendableSet";
    constructor(entries: T[]) {
        'use sendable';
        super(...entries);
    }
}

function subSendableArrayTest() {
    print("Start Test subSendableArrayTest")

    try {
        const tmpArr = new SubSendableArray<number>(5, 12, 8, 130, 44);
        print("create SubSendableArray success " + tmpArr.length);
    } catch (err) {
        print("create SubSendableArray failed. err: " + err + ", code: " + err.code);
    }

    const array1 = new SubSendableArray<number>([5, 12, 8, 130, 44]);
    let index = 1;
    print(`An index of ${index} returns ${array1.at(index)}`);

    index = 2871622679;
    print(`An index of 2871622679 returns ${array1.at(index)}`);

    print("" + array1.includes(2));
    const found = array1.find((element: number) => element > 10);

    print("" + found);
    let iterator = array1.values();
    for (const value of iterator) {
        print("" + value);
    }

    iterator = array1.keys();
    for (const key of iterator) {
        print("" + key);
    }

    try {
        print("" + array1.findIndex(array1.length - 1));
    } catch (err) {
        print("findIndex failed. err: " + err + ", code: " + err.code);
    }

    array1.fill(0, 2, 4);
    print(array1);

    array1.pop();
    print(array1.length);

    array1.push(10);

    print(array1.join());

    try {
        let a1 = array1.slice(1.5, 4);
        print("slice(1.5, 4) element success");
        print(a1);
    } catch (err) {
        print("slice element failed. err: " + err + ", code: " + err.code);
    }
}

 function allApiTest() {
    let array = new SendableArray<number>(1, 2, 3, 4);
    array.__proto__["slice"]();
    print(array.__proto__["at"](1));
    const iterator = array.__proto__["entries"]();
    for (const [key, value] of iterator) {
        print("" + key + "," + value);
    }
    print(array.__proto__["fill"](1));
    print(array.__proto__["filter"]((element: number) => element > 10));
    print(array.__proto__["find"]((element: number) => element > 10));
    print(array.__proto__["findIndex"]((element: number) => element > 13));
    print(array.__proto__["forEach"]((element: number) => print(element)));
    print(array.__proto__["includes"](1));
    print(array.__proto__["containsAll"]([]));
    print(array.__proto__["retainAll"]([]));
    print(array.__proto__["retainAll"]((element: number) => element > 10));
    print(array.__proto__["indexOf"](1));
    print(array.__proto__["join"]());
    print(array.__proto__["keys"]());
    print(array.__proto__["map"]<string>((x: number) => x + x));
    print(array.__proto__["pop"]());
    try {
        array.__proto__["reduce"]((acc: number, currValue: number) => acc + currValue);
    } catch {
        print("__proto__ call reduce error");
    }
    try {
        array.__proto__["shift"]();
    } catch {
        print("__proto__ call shift error");
    }
    print(array.__proto__["sort"]());
    print(array.__proto__["toString"]());
    print(array.__proto__["toLocaleString"]());
    print(array.__proto__["unshift"]());
    print(array.__proto__["values"]());
    print(array.__proto__["shrinkTo"](2));
    try {
        array.__proto__["extendTo"](6, 0);
    } catch {
        print("__proto__ call extendTo error")
    }

    try {
        array.__proto__["splice"](1, 0, 5, 7);
    } catch {
        print("__proto__ call splice error")
    }
    print(array.__proto__["every"]((element: number) => element > 10));
    print(array.__proto__["some"]((element: number) => element > 10));
    print(array.__proto__["lastIndexOf"]((element: number) => element > 10));
    print(array.__proto__["copyWithin"](1));
    print(array.__proto__["reduceRight"]((acc: number, currValue: number) => acc + currValue, undefined));
    print(array.__proto__["reverse"]());
    print(array.__proto__["findLast"]((element: number) => element > 10));
    print(array.__proto__["findLastIndex"]((element: number) => element > 10));
    print(array.__proto__["concat"](array));
    print(array.__proto__["push"](1));
}

at()

entries()

keys()

values()

find();

includes();

containsAll();
retainAll();

index();

fill();

pop();

randomUpdate();

randomGet();

randomAdd();
create();
from();
fromTemplate();
length();
push();
concat();
join()
shift()
unshift()
slice()
sort()
indexOf()
forEach()
map()
filter()
reduce()
splice()
staticCreate()
readonlyLength()
shrinkTo()
extendTo()
indexAccess()
indexStringAccess()
print("Start Test testForIC")
for (let index: number = 0; index < 100; index++) {
    testForIC(index)
}

print("Start Test testStringForIC")
for (let index: number = 0; index < 100; index++) {
    testStringForIC(index)
}

arrayFrozenTest()
sharedArrayFrozenTest()
arrayFromSet()
arrayFromNormalMap()
arrayFromSendableMap();
arrayFromNotArray();

derivedSlice();
derivedSort();
derivedForEach();
derivedMap()
derivedFill()
readOutOfRange()
forOf();
sharedArrayConstructorTest()
fromArrayConstructorTest()
DefinePropertyTest()

SomeTest()
EveryTest()
isArrayTest();
lastIndexOfTest();
ofTest();
copyWithinTest();

findLast();
findLastIndex();
reduceRight();
reverse();

toStringTest();
toLocaleStringTest();
subSendableArrayTest();
allApiTest();
