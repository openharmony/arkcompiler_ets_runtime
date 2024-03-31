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
 * @tc.name:sharedarray
 * @tc.desc:test sharedarray
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

SharedArray.from<string>(["a", "r", "k"])

function at() {
    print("Start Test at")
    const array1 = new SharedArray<number>(5, 12, 8, 130, 44);
    let index = 2;
    print(`An index of ${index} returns ${array1.at(index)}`); // An index of 2 returns 8

    index = -2;
    print(`An index of ${index} returns ${array1.at(index)}`); // An index of -2 returns 130

    index = 200;
    print(`An index of ${index} returns ${array1.at(index)}`); // An index of 200 returns undefined
}

function entries() {
    print("Start Test entries")
    const array1 = new SharedArray<string>('a', 'b', 'c');
    const iterator = array1.entries();
    for (const key of iterator) {
        print("" + key); // 0, 1, 2
    }
}

function keys() {
    print("Start Test keys")
    const array1 = new SharedArray<string>('a', 'b', 'c');
    const iterator = array1.keys();
    for (const key of iterator) {
        print("" + key); // 0, 1, 2
    }
}

function values() {
    print("Start Test values")
    const array1 = new SharedArray<string>('a', 'b', 'c');
    const iterator = array1.values();
    for (const value of iterator) {
        print("" + value); // a, b, c
    }
}

function find() {
    print("Start Test find")
    const array1 = new SharedArray<number>(5, 12, 8, 130, 44);

    const found = array1.find((element: number) => element > 10);
    print("" + found); // 12

    const array2 = new SharedArray<SuperClass>(new SubClass(5), new SubClass(32), new SubClass(8), new SubClass(130), new SubClass(44));
    const result: SubClass | undefined = array2.find<SubClass>((value: SuperClass, index: number, obj: SharedArray<SuperClass>) => value instanceof SubClass);
    print((new SubClass(5)).strProp); // 5
}

function includes() {
    print("Start Test includes")
    const array1 = new SharedArray<number>(1, 2, 3);
    print("" + array1.includes(2)); // true

    const pets = new SharedArray<string>('cat', 'dog', 'bat');
    print("" + pets.includes('cat')); // true

    print("" + pets.includes('at')); // false
}

function index() {
    print("Start Test index")
    const array1 = new SharedArray<number>(5, 12, 8, 130, 44);
    const isLargeNumber = (element: number) => element > 13;
    print("" + array1.findIndex(isLargeNumber)); // 3

}

function fill() {
    print("Start Test fill")
    const array1 = new SharedArray<number>(1, 2, 3, 4);
    array1.fill(0, 2, 4);
    print(array1); // [1, 2, 0, 0]

    array1.fill(5, 1);
    print(array1); // [1, 5, 5, 5]

    array1.fill(6);
    print(array1) // [6, 6, 6, 6]
}

// remove
function pop() {
    print("Start Test pop")
    const sharedArray = new SharedArray<number>(5, 12, 8, 130, 44);
    print("poped: " + sharedArray.pop());
}

// update
function randomUpdate() {
    print("Start Test randomUpdate")
    const sharedArray = new SharedArray<number>(5, 12, 8, 130, 44);
    sharedArray[1] = 30
    print(sharedArray[1]);
}

//  get
function randomGet() {
    print("Start Test randomGet")
    const sharedArray = new SharedArray<number>(5, 12, 8, 130, 44);
    sharedArray.at(0)
    print(sharedArray);
}

// add
function randomAdd() {
    print("Start Test randomAdd")
    const sharedArray = new SharedArray<number>(5, 12, 8);
    try {
        sharedArray[4000] = 7;
    } catch (err) {
        print("add element by index access failed. err: " + err + ", code: " + err.code);
    }
}

function create(): void {
    print("Start Test create")
    let arkTSTest: SharedArray<number> = new SharedArray<number>(5);
    let arkTSTest1: SharedArray<number> = new SharedArray<number>(1, 3, 5);
}

function from(): void {
    print("Start Test from")
    print(SharedArray.from<string>(["A", "B", "C"]));
    try {
        print(SharedArray.from<string>(["E", ,"M", "P", "T", "Y"]));
    } catch (err) {
        print("Create from empty element list failed. err: " + err + ", code: " + err.code);
    }
    const source = new SharedArray<undefined>(undefined, undefined, 1);
    try {
        print("Create from sendable undefined element list success. arr: " + SharedArray.from<string>(source));
    } catch (err) {
        print("Create from sendable undefined element list failed. err: " + err + ", code: " + err.code);
    }
}

function fromTemplate(): void {
    print("Start Test fromTemplate")
    let artTSTest1: SharedArray<string> = SharedArray.from<Number, string>([1, 2, 3], (x: number) => "" + x);
    print("artTSTest1: " + artTSTest1);
    let arkTSTest2: SharedArray<string> = SharedArray.from<Number, string>([1, 2, 3], (item: number) => "" + item); // ["1", "Key", "3"]
    print("arkTSTest2: " + arkTSTest2);
}

function length(): void {
    print("Start Test length")
    let array: SharedArray<number> = new SharedArray<number>(1, 3, 5);
    print("Array length: " + array.length);
    array.length = 50;
    print("Array length after changed: " + array.length);
}

function push(): void {
    print("Start Test push")
    let array: SharedArray<number> = new SharedArray<number>(1, 3, 5);
    array.push(2, 4, 6);
    print("Elements pushed: " + array);
}

function concat(): void {
    print("Start Test concat")
    let array: SharedArray<number> = new SharedArray<number>(1, 3, 5);
    let arkTSToAppend: SharedArray<number> = new SharedArray<number>(2, 4, 6);
    let arkTSToAppend1: SharedArray<number> = new SharedArray<number>(100, 101, 102);

    print(array.concat(arkTSToAppend)); // [1, 3, 5, 2, 4, 6]
    print(array.concat(arkTSToAppend, arkTSToAppend1));
    array.concat(200);
    array.concat(201, 202);
}

function join(): void {
    print("Start Test join")
    const elements = new SharedArray<string>('Fire', 'Air', 'Water');
    print(elements.join());
    print(elements.join(''));
    print(elements.join('-'));
}

function shift() {
    print("Start Test shift")
    const array1 = new SharedArray<number>(2, 4, 6);
    print(array1.shift());

    const emptyArray = new SharedArray<number>();
    print(emptyArray.shift());
}

function unshift() {
    print("Start Test unshift")
    const array = SharedArray.from<number>([1, 2, 3]);
    print(array.unshift(4, 5));
}

function slice() {
    print("Start Test slice")
    const animals = new SharedArray<string>('ant', 'bison', 'camel', 'duck', 'elephant');
    print(animals.slice());
    print(animals.slice(2));
    print(animals.slice(2, 4));
}

function sort() {
    print("Start Test sort")
    const months = new SharedArray<string>('March', 'Jan', 'Feb', 'Dec');
    print(months.sort());

    const array1 = [1, 30, 4, 21, 10000];
    print(array1.sort());

    array1.sort((a: number, b: number) => a - b);
}

function indexOf() {
    print("Start Test indexOf")
    const beasts = new SharedArray<string>('ant', 'bison', 'camel', 'duck', 'bison');
    print(beasts.indexOf('bison')); // Expected: 1
    print(beasts.indexOf('bison', 2)) // Expected: 4
    print(beasts.indexOf('giraffe')) // Expected： -1
}

function forEach() {
    print("Start Test forEach")
    const array = new SharedArray<string>('a', 'b', 'c');
    array.forEach((element: string) => print(element)); // a <br/> b <br/>  c

    array.forEach((element: string, index: number, array: SharedArray<string>) => print(`a[${index}] = ${element}, ${array[index]}`))
}

function map() {
    print("Start Test map")
    const array = new SharedArray<number>(1, 4, 9, 16);
    print(array.map<string>((x: number) => x + x));
}

function filter() {
    print("Start Test filter")
    const words = new SharedArray<string>('spray', 'elite', 'exuberant', 'destruction', 'present');
    print(words.filter((word: string) => word.length > 6))
    const array2 = new SharedArray<SuperClass>(new SubClass(5), new SuperClass(12), new SubClass(8), new SuperClass(130), new SubClass(44));
    const result = array2.filter<SubClass>((value: SuperClass, index: number, obj: Array<SuperClass>) => value instanceof SubClass);
    result.forEach((element: SubClass) => print(element.num)); // 5, 8, 44
}

function reduce() {
    print("Start Test reduce")
    const array = new SharedArray<number>(1, 2, 3, 4);
    print(array.reduce((acc: number, currValue: number) => acc + currValue)); // 10

    print(array.reduce((acc: number, currValue: number) => acc + currValue, 10)); // 20

    print(array.reduce<string>((acc: number, currValue: number) => "" + acc + " " + currValue, "10")); // 10, 1, 2, 3, 4
}

function staticCreate() {
    print("Start Test staticCreate")
    const array = SharedArray.create<number>(10, 5);
    print(array);
    try {
        const array = SharedArray.create<number>(5);
        print("Create with without initialValue success.");
    } catch (err) {
        print("Create with without initialValue failed. err: " + err + ", code: " + err.code);
    }
    try {
        const array = SharedArray.create<number>(-1, 5);
        print("Create with negative length success.");
    } catch (err) {
        print("Create with negative length failed. err: " + err + ", code: " + err.code);
    }
    try {
        const array = SharedArray.create<number>(0x100000000, 5);
        print("Create with exceed max length success.");
    } catch (err) {
        print("Create with exceed max length failed. err: " + err + ", code: " + err.code);
    }
}

function readonlyLength() {
    print("Start Test readonlyLength")
    const array = SharedArray.create<number>(10, 5);
    print(array.length);
    array.length = 0;
    print(array.length);
}

function shrinkTo() {
    print("Start Test shrinkTo")
    const array = new SharedArray<number>(5, 5, 5, 5, 5, 5, 5, 5, 5, 5);
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
    const array = SharedArray.create<number>(5, 5);
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
    const array = new SharedArray<number>(1, 3, 5, 7);
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
        array.forEach((key: number, _: number, array: SharedArray) => {
            array[key + array.length];
        });
    } catch (err) {
        print("read element while iterate array fail. err: " + err + ", errCode: " + err.code);
    }
    try {
        array.forEach((key: number, _: number, array: SharedArray) => {
            array[key + array.length] = 100;
        });
    } catch (err) {
        print("write element while iterate array fail. err: " + err + ", errCode: " + err.code);
    }
}

function indexStringAccess() {
    print("Start Test indexStringAccess")
    const array = new SharedArray<number>(1, 3, 5, 7);
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
        array.forEach((key: number, _: number, array: SharedArray) => {
            array["" + key + array.length];
        });
    } catch (err) {
        print("String index read element while iterate array fail. err: " + err + ", errCode: " + err.code);
    }
    try {
        array.forEach((key: number, _: number, array: SharedArray) => {
            array["" + key + array.length] = 100;
        });
    } catch (err) {
        print("String index write element while iterate array fail. err: " + err + ", errCode: " + err.code);
    }
}

function testForIC(index: number) {
    const array = new SharedArray<number>(1, 3, 5, 7);
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
    const array = new SharedArray<number>(1, 3, 5, 7);
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