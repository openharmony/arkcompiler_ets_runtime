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

let arr = Array.from("abcd");
print(arr);
arr = Array.from("abcd");
print(arr);
arr[1] = 'e';
print(arr);
arr = Array.from("abcd");
print(arr);

arr = Array.from("01234567890123456789012");
print(arr)
arr = Array.from("方舟")
print(arr);
arr = Array.from("方舟")
print(arr);
arr = Array.from("")
print(arr.length)
arr[0] = 'a'
arr = Array.from("")
print(arr.length)

var src = new Uint8Array(10000);
for(var i = 0; i < 10000; i++)
{
    src[i] = 1;
}
arr = Array.from(src);
print(arr[666]);
print(arr[999]);
print(arr.length);

const v1 = new Map();
print(Array.from(v1.keys()))

{
    let mp=new Map();
    let mpIter = mp.entries();
    mpIter.__proto__=[1,2,3,4];
    let res=Array.from(mpIter);
    print(res);
}

{
    class MyArray1 extends Array {
        constructor(...args) {
            super(...args);
            return {};
        }
    }
    let arr1 = MyArray1.from([1,2,3,4]);
    print(JSON.stringify(arr1));
    
    class MyArray2 extends Array {
        constructor(...args) {
            super(...args);
            return new Proxy({}, {
                get(o, k) {
                    print("get",k);
                    return o[k];
                 },
                set(o, k, v) { 
                    print("set",k);
                    return o[k]=v;
                },
                defineProperty(o, k, v) {
                    print("defineProperty",k);
                    return Object.defineProperty(o,k,v);
                 }
            });
        }
    }
    let arr2 = MyArray2.from([1,2,3,4]);
    print(JSON.stringify(arr2));

    class MyArray3 extends Array {
        constructor(...args) {
            super(...args);
            return new Proxy(this, {
                get(o, k) {
                    print("get",k);
                    return o[k];
                 },
                set(o, k, v) { 
                    print("set",k);
                    return o[k]=v;
                },
                defineProperty(o, k, v) {
                    print("defineProperty",k);
                    return Object.defineProperty(o,k,v);
                 }
            });
        }
    }
    let arr3 = MyArray3.from([1,2,3,4]);
    print(JSON.stringify(arr3));
}

{
    let arrIterBak = Array.prototype[Symbol.iterator];
    let obj = {
        get length() {
            print("get length");
            return 10;
        },
        set length(x) {
            print("set length", x);
            return true;
        },
        get 0() {
            print('get 0');
            return 0;
        },
        get 1() {
            print('get 1');
            return 1;
        },
        get 2() {
            print('get 2');
            return 2;
        },
        get [Symbol.iterator]() {
            print("get iterator");
            return arrIterBak;
        }
    }
    let res = Array.from(obj);
    print(JSON.stringify(res));
}

{
    let arr = [1, 2, 3, 4, 5, 6];
    Object.defineProperty(arr, 0, {
        get() {
            print("get 0");
            arr.pop();
            return "x";
        }
    });
    let res = Array.from(arr);
    print(JSON.stringify(res))
}
{
    let arrIterBak = Array.prototype[Symbol.iterator];
    let arr = new Object(1);
    arr[1] = 1;
    arr.length = 10;
    arr[Symbol.iterator] = arrIterBak;
    print(arr.constructor)
    let res = Array.from(arr);
    print(JSON.stringify(res))
}
{
    let arrIterBak = Array.prototype[Symbol.iterator];
    Number.prototype.__proto__ = {
        get length() {
            print("get length");
            return 10;
        },
        set length(x) {
            print("set length", x);
            return true;
        },
        get 0() {
            print('get 0');
            return 0;
        },
        get 1() {
            print('get 1');
            return 1;
        },
        get 2() {
            print('get 2');
            return 2;
        },
        get [Symbol.iterator]() {
            print("get iterator");
            return arrIterBak;
        }
    };
    let arr = 1
    let res = Array.from(arr);
    print(JSON.stringify(res))
}

{
    let arr = [1,2,3];
    let res = Array.from(arr.values());
    print(JSON.stringify(res));
}

// array.from by arrayLike with mapFunc
{
    let res = Array.from({length : 3}, () => {});
    print(JSON.stringify(res));
}
  
{
    let res = Array.from({length : 3}, () => ({}));
    print(JSON.stringify(res));
}
  
{
    let res = Array.from({length : 3}, () => []);
    print(JSON.stringify(res));
}
  
{
    let res = Array.from({length : 3}, () => [1,2,3]);
    print(JSON.stringify(res));
}
  
{
    let res = Array.from({length : 3}, () => 0);
    print(JSON.stringify(res));
}
  
{
    let num = 1;
    let len = 1025;
    let res = Array.from({length : len}, () => num);
    print(res.length == len);
    let flag = true;
    for (let i = 0; i < res.length; ++i) {
      if (res[i] != num) {
        flag = false;
        break;
      }
    }
    if (flag) {
      print("get JSArray from arrayLike Success");
    }
}

{
    function example() {
      let res = Array.from(arguments);
      print(JSON.stringify(res));
    }
    example(1, 2, 3);
}
  
{
    let arrayLike = {0:1.1, 1:12, 2:'ss', length: 3}
    let res = Array.from(arrayLike, x => x + x);
    print(JSON.stringify(res));
}
  
{
    let res = Array.from({length : 3}, (_, index) => [index * 2]);
    print(JSON.stringify(res));
}

{
    const nonConstructor = {}
    let res = Array.from.call(nonConstructor, {length : 3}, (_, index) => [index * 2]);
    print(JSON.stringify(res));
}

//array.from by JSArray
{
    const nonConstructor = {}
    let num = 1
    let len = 1025 // may transfer to dictionary elements type
    let myArray = new Array(1025).fill(num)
    let res = Array.from.call(nonConstructor, myArray);
    print(res.length == len);
    let flag = true;
    for (let i = 0; i < res.length; ++i) {
      if (res[i] != num || res.at(i) != num) {
        flag = false;
        break;
      }
    }
    if (flag) {
      print("get JSArray from JSArray Success!")
    }
}
  
{
    const nonConstructor = {}
    let myArray = new Array(1,2,3,4,5)
    let res = Array.from.call(nonConstructor, myArray);
    print(JSON.stringify(res));
}
  
{
    let res = Array.from([1,2,3,4,5]);
    print(JSON.stringify(res));
}

// test for String with mapFunc
{
    let str = 'a'.repeat(10)
    let res = Array.from(str, x => x + 's');
    print(JSON.stringify(res));
}
  
{
    let len = 1025
    const head = 'h'
    const tail = '_tail'
    let str = head.repeat(len)
    let res = Array.from(str, x => x + tail);
    let flag = true;
    for (let i = 0; i < res.length; ++i) {
      if (res[i] != head + tail) {
        flag = false;
        break;
      }
    }
    if (res.length == len && flag) {
      print("result check successfully");
    } else {
      print("result check failed");
    }
}
  
// test for Set with mapFunc
{
    let set = new Set(['test', 'for', 'array', 'from', 'set'])
    let res = Array.from(set, x => x);
    print(JSON.stringify(res));
}
  
// test for Map with mapFunc
{
    let map = new Map([[1, 'test'], [2, 'for'], [3, 'array'], [4, 'from'], [5, 'map']]);
    let res = Array.from(map, x => x);
    print(JSON.stringify(res));
}
  
// test for TypedArray with mapFunc
{
    let mapFunc = x => x + x;
    let uint8Array = new Uint8Array([1, 2, 3, 4, 5, 6]);
    let res = Array.from(uint8Array, mapFunc);
    print(JSON.stringify(res));
}
  
{
    let mapFunc = x => x + x;
    let uint16Array = new Uint16Array([1, 2, 3, 4, 5, 6]);
    let res = Array.from(uint16Array, mapFunc);
    print(JSON.stringify(res));
}
  
{
    let mapFunc = x => x + x;
    let uint32Array = new Uint32Array([1, 2, 3, 4, 5, 6]);
    let res = Array.from(uint32Array, mapFunc);
    print(JSON.stringify(res));
}
  
{
    let mapFunc = x => x + x;
    let float32Array = new Float32Array([1, 2, 3, 4, 5, 6]);
    let res = Array.from(float32Array, mapFunc);
    print(JSON.stringify(res));
}
  
{
    let mapFunc = x => x + x;
    let float64Array = new Float64Array([1, 2, 3, 4, 5, 6]);
    let res = Array.from(float64Array, mapFunc);
    print(JSON.stringify(res));
}