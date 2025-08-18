/*
 * Copyright (c) 2023 Huawei Device Co., Ltd.
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

class DeepProxy {
    constructor(obj, handler) {
        return new Proxy(obj, handler);
    }
}
class ClassB {
    constructor(n) {
        this.n = 0;
        this.n = n;
    }
}

let nextFreeId = 0;
class ClassA {
    constructor(a, b) {
        this.a = a;
        this.b = new ClassB(b);
        this.id = nextFreeId++;
    }
}

// Testing the proxy situation.
let data1 = [new ClassA(1, 10), new ClassA(3, 30), new ClassA(4, 40), new ClassA(2, 20), new ClassA(11, 250)];
let objHandler1 = new DeepProxy(data1, {});
print(JSON.stringify(objHandler1));
objHandler1.sort((a, b) => {
    return a.b.n - b.b.n;
})
print(JSON.stringify(objHandler1));

// Testing cases with both proxy and hole.
let data2 = [new ClassA(1, 10), , new ClassA(3, 30), , new ClassA(4, 40), new ClassA(2, 20), new ClassA(11, 250)];
let objHandler2 = new DeepProxy(data2, {
    deleteProperty(target, prop) {
        print(`delete ${prop.toString()}`);
        return Reflect.deleteProperty(target, prop);
    }
});
objHandler2.sort((a, b) => {
    return a.b.n - b.b.n;
})
print(JSON.stringify(objHandler2));

/*
 * Test Case Description:
 * 1. This use case is used to verify the logical processing order of the binary insertion sorting algorithm.
 * 2. If there are any changes to the use case, please confirm if the use case needs to be modified.
 */
let arr1 = [1, 3, 2];
arr1.sort((a, b) => {
    return a - b;
});
print(JSON.stringify(arr1));

/*
 * Test Case Description:
 * 1. This use case is used to verify the logical processing order of the quick sorting algorithm.
 * 2. If there are any changes to the use case, please confirm if the use case needs to be modified.
 */
for (let i = 0; i < 100; i++) {
    arr1[i] = i;
}
arr1[0] = 99;
arr1[99] = 0;
arr1[49] = 50;
arr1[50] = 49;
arr1.sort((a, b) => {
    return a - b;
})
print(JSON.stringify(arr1));

// Modification of objects during the comparison process.
let arr2 = [1, 3, 2];
arr2.sort((a, b) => {
    if (a == 1 || b == 1) {
        arr2[0] == 2;
    }
    return a - b;
});
print(JSON.stringify(arr2));

let arr3 = [1, 3, 2];
arr3.sort((a, b) => {
    if (a == 1 || b == 1) {
        arr3[4] == 2;
    }
    return a - b;
});
print(JSON.stringify(arr3));

// Testing the situation where this is an Object
let obj1 = {0: 1, 1: 3, a: 6, 2: 2, length: 3};
Array.prototype.sort.call(obj1, (a, b) => {
    return a - b;
});
print(JSON.stringify(obj1));

let obj2 = {0: 1, 1: 3, a: 6, 2: 2, length: 3};
Array.prototype.sort.call(obj2, (a, b) => {
    if (a == 1 || b == 1) {
        obj2.a = 60;
    }
    return a - b;
});
print(obj2.a == 60);
print(JSON.stringify(obj2));

let obj3 = {0: 1, 1: 3, a: 6, 2: 2, length: 2};
Array.prototype.sort.call(obj3, (a, b) => {
    return a - b;
});
print(obj3[1] == 3)
print(JSON.stringify(obj3));

let obj4 = {0: 1, 1: 3, a: 6, 3: 2, length: 4};
Array.prototype.sort.call(obj4, (a, b) => {
    return a - b;
});
print(obj4[2] == 3)
print(JSON.stringify(obj4));

// Test if this is a Map type;
let map1 = new Map();
map1.set(0, 1);
map1.set(1, 3);
map1.set(2, 2);
map1.set("a", 6);
map1.set("length", 3);
Array.prototype.sort.call(map1, (a, b) => {
    return a - b;
});
print(JSON.stringify(map1));

let map2 = new Map();
map2.set(0, 1);
map2.set(1, 3);
map2.set(2, 2);
map2.set("a", 6);
map2.set("length", 3);
Array.prototype.sort.call(map2, (a, b) => {
    if (a == 1 || b == 1) {
        map2.set("a", 60);
    }
    return a - b;
});
print(JSON.stringify(map2));

// Test prototype
let child1 = [1, 3, 2];
let proto1 = [4, 7, 5];
child1.__proto__ = proto1;
child1.sort((a, b) => {
    return a - b;
});
print(JSON.stringify(child1));

let child2 = [1, , 2];
child2.__proto__ = proto1;
child2.sort((a, b) => {
    return a - b;
});
print(child2.hasOwnProperty('1'));
print(JSON.stringify(child2));

let child3 = [1, 3, 2];
let proto2 = [4, , 5];
child3.__proto__ = proto2;
child3.sort((a, b) => {
    return a - b;
});
print(JSON.stringify(child3));

let child4 = [1, , 2];
child4.__proto__ = proto2;
child4.sort((a, b) => {
    return a - b;
});
print(child4.hasOwnProperty('2'));
print(JSON.stringify(child4));

var test1 = [-321, 65, 0, -3215, 653, 650, -3210, -2147483648, 2147483647];
print(test1.sort());


var arr4 = new Array(5);
arr4[0] = 93;
arr4[1] = 930;
arr4[2] = -45;
arr4[3] = 44;
arr4[4] = 0;
print(arr4.sort(function(a, b){
    a--;
    b--;
    return b-a;
}));

var arr5 = [3, 1, 4];
arr5.sort((a, b) => {
    if (a == 1 || b == 1) {
        arr5[0] = 6;
    }
    return a - b;
});
print(arr5);

Object.defineProperty(Array.prototype, "tt", {
    value:37,
    writable:false,
});

var arr6 = new Array(5);
arr6[0] = 93;
arr6[2] = -45;
arr6[3] = "djs";
arr6[4] = 0;
print(arr6.sort());

var arr7 = [1];
print(arr7.sort());

var res1 = Array.prototype.sort.call("m", Uint8Array);
print(res1);

try {
    Array.prototype.sort.call("mm", Uint8Array);
} catch (e) {
    print(e.name);
}

const o1 = {
    ..."654"
};
const arr8 = [1, 2, 3];
const o2 = {
    __proto__: arr8,
    ...o1
};
o2.sort();
print(o2[0]);
print(o2[1]);
print(o2[2]);

const bigint64_array = new BigInt64Array();
const proxy = new Proxy([1, 2, 3], bigint64_array);
try {
    proxy.sort();
} catch (e) {
    print(e.name);
}

try {
    const vSort = new Float64Array(Float64Array);
    vSort.__proto__.sort();
} catch (e) {
    print(e.message);
}

const v4 = [1, 255];
class C5 extends Int16Array{
    toString(a7, a8, a9, a10) {
        super.includes();
    }
}
try {
    const v12 = new C5();
    v4.sort(C5.prototype.toString);
} catch (e) {
    print(e.message);
}
const items = [
    { name: "Edward", value: 21 },
    { name: "Sharpe", value: 37 },
    { name: "And", value: 45 },
    { name: "The", value: -12 },
    { name: "Magnetic", value: 13 },
    { name: "Zeros", value: 37 },
  ];

items.sort((a, b) => a.value - b.value);
print(JSON.stringify(items));

items.sort((a, b) => {
    const nameA = a.name.toUpperCase();
    const nameB = b.name.toUpperCase();
    if (nameA < nameB) {
      return -1;
    }
    if (nameA > nameB) {
      return 1;
    }
    return 0;
  });

print(JSON.stringify(items));

const numbers = [3, 1, 4, 1, 5];
const sorted = numbers.sort((a, b) => a - b);
sorted[0] = 10;
print(numbers[0]); // 10

const students = [
    { name: "Alex", grade: 15 },
    { name: "Devlin", grade: 15 },
    { name: "Eagle", grade: 13 },
    { name: "Sam", grade: 14 },
];
// stable
students.sort((firstItem, secondItem) => firstItem.grade - secondItem.grade);
print(JSON.stringify(students));
const v2 =[];
class C3{};
v2.__proto__ = C3;
let arr = new Array(518);
for(let i=0;i<518;i++){
    arr[i]=""+i;
}
arr[512]="valueOf";
arr[513]="p0";
arr[514]="p1";
arr[515]="p2";
arr[516]="p3";
arr[517]="p4";
arr.sort();

//for cmp return type is double
let arr34 = [2.63, 1.67];
print(arr34.sort((a, b)=> a - b));
print("sort Test Success!");

// String toSorted
{
	let array1 = new Array();
    for (let i = 0; i < 1; i++) array1[i] = `string.${i}`;
    print(array1.toSorted());
    
    array1 = new Array();
    for (let i = 0; i < 2; i++) array1[i] = `string.${i}`;
    print(array1.toSorted());

    array1 = new Array();
    for (let i = 0; i < 2; i++) array1[i] = `string.${1 - i}`;
    print(array1.toSorted());
  
    array1 = new Array();
    array1[0] = `a`;
    array1[1] = `b`;
    array1[2] = `c`;
    array1[3] = `d`;
    array1[4] = `e`;
    array1[5] = `f`;
    array1[6] = `g`;
    print(array1.toSorted());

    array1 = new Array();
    array1[6] = `a`;
    array1[5] = `b`;
    array1[4] = `c`;
    array1[3] = `d`;
    array1[2] = `e`;
    array1[1] = `f`;
    array1[0] = `g`;
    print(array1.toSorted());
}

// Test sort if array has hole
function HoleSort()
{
  let sortNumber= [];
  for (let i = 0; i < 10; i++) {
    sortNumber.push(parseInt(i));
  }
  sortNumber[100] = parseInt(100);
  print(sortNumber.length);
  print(sortNumber);

  sortNumber.sort((a, b) => {
    return a < b;
  });

  print(sortNumber.length);
  print(sortNumber);
}

HoleSort();

let sortNumber = [0,3,2,,4,5,6];
sortNumber.sort((a,b) => {
    sortNumber[10000] = 1;
    return a-b;
});
print(sortNumber.length);
print(sortNumber);

let sortNumber2 = [0,3,2,,4,5,6];
sortNumber2.sort((a,b) => {
    sortNumber2[3] = 1;
    return a-b;
});
print(sortNumber2.length);
print(sortNumber2);

let sortNumber3 = [0,3,2,,4,5,6];
sortNumber3.sort((a,b) => {
    sortNumber3 = 1;    // stlexvar
    return a-b;
});
print(sortNumber3.length);
print(sortNumber3[0]);
print(sortNumber3[2]);
print(sortNumber3[4]);
print(sortNumber3[6]);

let sortNumber4 = [0,3,2,,4,5,6];
sortNumber4.sort((a,b) => {
    sortNumber4.push(1);
    return a-b;
});
print(sortNumber4.length);
print(sortNumber4[0]);
print(sortNumber4[2]);
print(sortNumber4[4]);
print(sortNumber4[6]);

let sortNumber5 = [-1, 2, 4, 1, 0];
sortNumber5.sort((x, y) => {
    Object.defineProperty(sortNumber5, '2', {
        get() {
            print("get second element:");
            return this.value;
        },
        set(newValue) {
            print("set second element:", newValue);
            this.value = newValue;
        }
    });
    return x - y;
})
print(sortNumber5.length);
print(sortNumber5.value);
print(sortNumber5);

let sortNumber6 = [-1, 2, 4, 1, 0];
sortNumber6.sort((x, y) => {
    Object.defineProperty(sortNumber6, '100', {
        get() {
            print("get 10000th element:");
            return this.value;
        },
        set(newValue) {
            print("set 10000th element:", newValue);
            this.value = newValue;
        },
        configurable: true  // 允许重新定义
    });
    return x - y;
})
print(sortNumber6.length);
print(sortNumber6.value);
print(sortNumber6);

let sortNumber7 = [0,3,2,,4,5,6];
sortNumber7.sort((a,b) => {
    sortNumber7.pop();
    return a-b;
});
print(sortNumber7.length);
print(sortNumber7[0]);
print(sortNumber7[2]);
print(sortNumber7[4]);
print(sortNumber7[6]);
// double sort
{
    let array1 = new Array();
    array1[0] = NaN;
    array1[1] = 0.0;
    array1[2] = 1;
    array1[3] = -0.1;
    array1[4] = Infinity;
    array1[5] = -Infinity;
    array1[10] = NaN;
    array1[11] = 0.0;
    array1[12] = 1;
    array1[13] = -0.1;
    array1[14] = Infinity;
    array1[15] = -Infinity;
    print(array1.toSorted());

    array1 = [5562684646268003, 0.005431, 0.00000019045, -79.39773355813419,
        1e21, 340000000000000000, 12.01234567890123456789, 0.000001234567890123456789, Infinity,
        -Infinity, 1.7976931348623157e+308, -1.7976931348623157e+308, 2.22507e-308,
        0.0000001234567890123456789, 3.4e21, 1.2e20, 1.2e0, 1.2e-6, 1.2e-7, NaN, -12.01234567890123456789,
        -0.000001234567890123456789, -0.0000001234567890123456789, -3.4e21, -1.2e20, -1.2e0, -1.2e-6, -1.2e-7,
        0.0, 0
    ];
    print(array1.toSorted());
}

{
    // double sort need barrier
    let arrayInt = new Array(3,2,1,0);
    print(arrayInt.sort());
    let arrayHoleInt = new Array(4);
    arrayHoleInt[1] = 4;
    arrayHoleInt[3] = 2;
    arrayHoleInt[4] = 1;
    print(arrayHoleInt.sort());

    let arrayNumber = new Array(3.1, 2.1, 1.1, 0);
    print(arrayNumber.sort());
    let arrayHoleNumber = new Array(4);
    arrayHoleNumber[1] = 4.1;
    arrayHoleNumber[3] = 2.1;
    arrayHoleNumber[4] = 1.1;
    print(arrayHoleNumber.sort());
}

{
    let array1;
    function arrayToString(arr) {
        return arr.map(e => {
            if (e === undefined) {
                return 'undefined'; 
            }
            return e == null ? "null" : e.toString();
        });
    }
    // object toString
    function objectToSorted(){
        let array1 = new Array();
        array1 = new Array();
        for (let i = 0; i < 5; i++) array1[i] = { ["obj" + i]: i };
        return array1.toSorted();
    }
    // has hole,undefined,false,true,null
    function specialValueToSorted() {
        array1 = new Array();
        array1[1] = undefined;
        array1[2] = false;
        array1[3] = true;
        array1[4] = null;
        return array1.toSorted();
    
    }
    // Symbol element toSorted
    function symbolToSorted() {
        array1 = new Array();
        for (let i = 0; i < 5; i++){
            array1[i] = {
                [Symbol.toPrimitive](hint) {
                    return 5 - i;
                },
            };
        }
        return array1.toSorted();
    }
    // number element toSorted
    function numberToSorted() {
       array1 = new Array();
        for (let i = 0; i < 5; i++) {
            array1[i] = 5-i;
        }
        return array1.toSorted(); 
    }
    // bigint element toSorted
    function bigIntToSorted() {
        array1 = new Array();
        array1[0] = 5n;
        array1[1] = 4n;
        array1[2] = 3n;
        array1[3] = 2n;
        array1[4] = 1n;
        return array1.toSorted(); 
    }
    // object rewrite toString element toSorted
    function objectRewriteToSorted() {
        class Cat{
        }
        Cat.prototype.toString = function () {
            return -1;
        }
        array1 = new Array();
        array1[0] = 5;
        array1[1] = 4;
        array1[2] = 3;
        array1[3] = 2;
        array1[4] = new Cat();
        return array1.toSorted(); 
    }
    // symbol throw error element toSorted
    function symbolThrowToSorted() {
        array1 = new Array();
        array1[0] = 5;
        array1[1] = 4;
        array1[2] = 3;
        array1[3] = 2;
        array1[4] = {
                [Symbol.toPrimitive](hint) {
                    throw new Error("Symbol.toPrimitive");
                }
            };
        return array1.toSorted(); 
    }

    // symbol return object element toSorted
    function symbolNotStringToSorted() {
        array1 = new Array();
        array1[0] = 5;
        array1[1] = 4;
        array1[2] = 3;
        array1[3] = 2;
        array1[4] = {
                [Symbol.toPrimitive](hint) {
                    return {};
                }
            };
        return array1.toSorted(); 
    }

    // object toString throw error toSorted
    function objectToStringThrowErrorToSorted() {
        class Cat{
        }
        Cat.prototype.toString = function () {
            throw new Error("toString");
        }
        array1 = new Array();
        array1[0] = 5;
        array1[1] = 4;
        array1[2] = 3;
        array1[3] = 2;
        array1[4] = new Cat();
        return array1.toSorted(); 
    }

    // object valueOf element toSorted
    function objectValueOfToSorted() {
        class Cat{
        }
        Cat.prototype.toString = function () {
            return {};
        }
        Cat.prototype.valueOf = function () {
            return -1;
        }
        array1 = new Array();
        array1[0] = 5;
        array1[1] = 4;
        array1[2] = 3;
        array1[3] = 2;
        array1[4] = new Cat();
        return array1.toSorted(); 
    }

    // object valueOf throw error toSorted
    function objectValueOfThrowErrorToSorted() {
        class Cat{
        }
        Cat.prototype.toString = function () {
            return {};
        }
        Cat.prototype.valueOf = function () {
            throw new Error("valueOf");
        }
        array1 = new Array();
        array1[0] = 5;
        array1[1] = 4;
        array1[2] = 3;
        array1[3] = 2;
        array1[4] = new Cat();
        return array1.toSorted(); 
    }

    // object valueOf toString return {} toSorted
    function objectNoStringToSorted() {
        class Cat{
        }
        Cat.prototype.toString = function () {
            return {};
        }
        Cat.prototype.valueOf = function () {
            return {};
        }
        array1 = new Array();
        array1[0] = 5;
        array1[1] = 4;
        array1[2] = 3;
        array1[3] = 2;
        array1[4] = new Cat();
        return array1.toSorted(); 
    }
    // Cannot convert a illegal value to a String; toSorted
    function CannotConvertIllegalValueToStringToSorted() {
        array1 = new Array();
        array1[0] = Symbol("1");
        array1[1] = Symbol("1");
        array1[2] = Symbol("1");
        array1[3] = Symbol("1");
        array1[4] = Symbol("1");
        return array1.toSorted(); 
    }

    // date hint sort
    function dateHintSort()
    {
        // default datehint string sort;
        let a = new Date('10 December 2019 14:48');
        let a1 = new Date('10 December 2019 14:48');
        let b = new Date('11 December 2019 14:48');
        let b1 = new Date('11 December 2019 14:48');
        let c = new Date('12 December 2019 14:48');
        let c1 = new Date('12 December 2019 14:48');
        let d = new Date('13 December 2019 14:48');
        let d1 = new Date('13 December 2019 14:48');
        let e = new Date('14 December 2019 14:48');
        let e1 = new Date('14 December 2019 14:48');
        let dateHint = [a, b, c, d, e];
        // Fri Dec 13 2019 14:48:00 GMT+0800,Sat Dec 14 2019 14:48:00 GMT+0800,Thu Dec 12 2019 14:48:00 GMT+0800,Tue Dec 10 2019 14:48:00 GMT+0800,Wed Dec 11 2019 14:48:00 GMT+0800
        print(dateHint.sort());
        a[Symbol.toPrimitive] = (hint)=>{
            return a1[Symbol.toPrimitive]('number');
        };
        b[Symbol.toPrimitive] = (hint)=>{
            return b1[Symbol.toPrimitive]('number');
        };
        c[Symbol.toPrimitive] = (hint)=>{
            return c1[Symbol.toPrimitive]('number');
        };
        d[Symbol.toPrimitive] = (hint)=>{
            return d1[Symbol.toPrimitive]('number');
        };
        e[Symbol.toPrimitive] = (hint)=>{
            return e1[Symbol.toPrimitive]('number');
        };
        print(dateHint.sort());
    }
    // [object Object],[object Object],[object Object],[object Object],[object Object]
    print(objectToSorted());
    // false,null,true,undefined,undefined
    print(arrayToString(specialValueToSorted()));
    // 1,2,3,4,5
    print(symbolToSorted());
    // 1,2,3,4,5
    print(numberToSorted());
    // 1,2,3,4,5
    print(bigIntToSorted());
    // -1,2,3,4,5
    print(objectRewriteToSorted());
    // Symbol.toPrimitive
    try {
        symbolThrowToSorted()
    } catch (e) {
        print(e.message);
    }
    // Cannot convert object to primitive value
    try {
        symbolNotStringToSorted();
    } catch (e) {
        print(e.message);
    }
    // toString
    try {
        objectToStringThrowErrorToSorted()
    } catch (e) {
        print(e.message);
    }
    // -1,2,3,4,5
    print(objectValueOfToSorted());
    // valueOf
    try {
        objectValueOfThrowErrorToSorted()
    } catch (e) {
        print(e.message);
    }
    // Cannot convert object to primitive value
    try {
        objectNoStringToSorted();
    } catch (e) {
        print(e.message);
    }
    // Cannot convert a illegal value to a String
    try {
        CannotConvertIllegalValueToStringToSorted();
    } catch (e) {
        print(e.message);
    }
    dateHintSort();
}


{
    print("---------------test for stable array sort where arr changeto dictionary array during sorting-----------")
    let arr = new Array(20);
    let haschang = false;
    for (let i = 0; i < arr.length; i++) {
        arr[i] = i;
    }
    arr[1] = {
        toString() {
            if (!haschang) {
                arr.length = 102400;
            }
            return -999;
        }
    }
    arr.sort();
    arr.splice(-1, 1);
    print("---------------test for stable array sort where arr changeto dictionary array during sorting end-------")
}
{
    print("---------------test for stable array sort where elements length change---------------------------------")
    let arr = new Array(20);
    let haschang = false;
    for (let i = 0; i < arr.length; i++) {
        arr[i] = i;
    }
    arr[1] = {
        toString() {
            if (!haschang) {
                arr.splice(-1, 1);
            }
            return -999;
        }
    }
    arr.sort();
    arr.splice(-1, 1)
    print("---------------test for stable array sort where elements length change end-----------------------------")
}
{
    print("---------------test for string array sort---------------------------------")
    let arr = new Array("aa","ab","ac","abc","abb","aba","a","b","c",);
    print(arr.toSorted());
    print("---------------test for string array sort end-----------------------------")
}

{
    let arr = [0, , 1];
    arr.sort((v1, v2) => {
        arr.shift();
    });
    print("test for array sort with shift Success!");
}
