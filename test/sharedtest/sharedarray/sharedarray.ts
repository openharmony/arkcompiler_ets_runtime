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
    const sharedArray = new SendableArray<number>(5, 12, 8, 130, 44);
    print("poped: " + sharedArray.pop());
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
}

function shift() {
    print("Start Test shift")
    const array1 = new SendableArray<number>(2, 4, 6);
    print(array1.shift());
    print(array1.length);

    const emptyArray = new SendableArray<number>();
    print(emptyArray.shift());
}

function unshift() {
    print("Start Test unshift")
    const array = SendableArray.from<number>([1, 2, 3]);
    print(array.unshift(4, 5));
    print(array.length);
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
}

at()

entries()

keys()

values()

find();

includes();

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
