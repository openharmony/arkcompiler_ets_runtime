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

/*
 * @tc.name: arrayfrom
 * @tc.desc: test Array.from
 * @tc.type: FUNC
 */

{
    let arr = Array.from("abcd");
    assert_equal(arr, ['a', 'b', 'c', 'd']);
    arr = Array.from("abcd");
    assert_equal(arr, ['a', 'b', 'c', 'd']);
    arr[1] = 'e';
    assert_equal(arr, ['a', 'e', 'c', 'd']);
    arr = Array.from("abcd");
    assert_equal(arr, ['a', 'b', 'c', 'd']);

    arr = Array.from("01234567890123456789012");
    assert_equal(arr, ['0','1','2','3','4','5','6','7','8','9','0','1','2','3','4','5','6','7','8','9','0','1','2']);
    arr = Array.from("方舟")
    assert_equal(arr, ["方", "舟"]);
    arr = Array.from("方舟")
    assert_equal(arr, ["方", "舟"]);
    arr = Array.from("")
    assert_equal(arr.length, 0);
    arr[0] = 'a'
    arr = Array.from("")
    assert_equal(arr.length, 0);
}

{
    var src = new Uint8Array(10000);
    for(var i = 0; i < 10000; i++)
    {
        src[i] = 1;
    }
    let arr = Array.from(src);
    assert_equal(arr[666], 1);
    assert_equal(arr[999], 1);
    assert_equal(arr.length, 10000);
}

{
    const v1 = new Map();
    assert_equal(Array.from(v1.keys()), []);
}

{
    let mp = new Map();
    let mpIter = mp.entries();
    mpIter.__proto__= [1,2,3,4];
    let res = Array.from(mpIter);
    assert_equal(res, [1, 2, 3, 4]);
}

{
    class MyArray1 extends Array {
        constructor(...args) {
            super(...args);
            return {};
        }
    }
    let arr1 = MyArray1.from([1,2,3,4]);
    assert_equal(JSON.stringify(arr1), "{\"0\":1,\"1\":2,\"2\":3,\"3\":4,\"length\":4}");
}

{
    class MyArray2 extends Array {
        constructor(...args) {
            super(...args);
            return new Proxy({}, {
                get(o, k) {
                    return o[k];
                },
                set(o, k, v) {
                    return o[k]=v;
                },
                defineProperty(o, k, v) {
                    return Object.defineProperty(o,k,v);
                }
            });
        }
    }
    let arr2 = MyArray2.from([1,2,3,4]);
    assert_equal(JSON.stringify(arr2), "{\"0\":1,\"1\":2,\"2\":3,\"3\":4,\"length\":4}");
}
{
    class MyArray3 extends Array {
        constructor(...args) {
            super(...args);
            return new Proxy(this, {
                get(o, k) {
                    return o[k];
                },
                set(o, k, v) { 
                    return o[k]=v;
                },
                defineProperty(o, k, v) {
                    return Object.defineProperty(o,k,v);
                }
            });
        }
    }
    let arr3 = MyArray3.from([1,2,3,4]);
    assert_equal(JSON.stringify(arr3), '[1,2,3,4]');
}

{
    let arrIterBak = Array.prototype[Symbol.iterator];
    let obj = {
        get length() {
            return 10;
        },
        set length(x) {
            return true;
        },
        get 0() {
            return 0;
        },
        get 1() {
            return 1;
        },
        get 2() {
            return 2;
        },
        get [Symbol.iterator]() {
            return arrIterBak;
        }
    }
    let res = Array.from(obj);
    assert_equal(res, [0, 1, 2,,,,,,,,])
}

{
    let arr = [1, 2, 3, 4, 5, 6];
    Object.defineProperty(arr, 0, {
        get() {
            arr.pop();
            return "x";
        }
    });
    let res = Array.from(arr);
    assert_equal(res, ['x', 2, 3, 4, 5]);
}

{
    let arrIterBak = Array.prototype[Symbol.iterator];
    let arr = new Object(1);
    arr[1] = 1;
    arr.length = 10;
    arr[Symbol.iterator] = arrIterBak;
    let res = Array.from(arr);
    assert_equal(res, [,1,,,,,,,,,]);
}

{
    let arrIterBak = Array.prototype[Symbol.iterator];
    Number.prototype.__proto__ = {
        get length() {
            return 10;
        },
        set length(x) {
            return true;
        },
        get 0() {
            return 0;
        },
        get 1() {
            return 1;
        },
        get 2() {
            return 2;
        },
        get [Symbol.iterator]() {
            return arrIterBak;
        }
    };
    let arr = 1
    let res = Array.from(arr);
    assert_equal(res, [0, 1, 2, ,,,,,,,]);
}

{
    let arr = [1,2,3];
    let res = Array.from(arr.values());
    assert_equal(res, [1, 2, 3]);
}

// array.from by arrayLike with mapFunc
{
    let res = Array.from({length : 3}, () => {});
    assert_equal(res, [,,,]);
}
  
{
    let res = Array.from({length : 3}, () => ({}));
    assert_equal(JSON.stringify(res), '[{},{},{}]');
}
  
{
    let res = Array.from({length : 3}, () => []);
    assert_equal(res, [[],[],[]]);
}
  
{
    let res = Array.from({length : 3}, () => [1,2,3]);
    assert_equal(res, [[1,2,3],[1,2,3],[1,2,3]]);
}
  
{
    let res = Array.from({length : 3}, () => 0);
    assert_equal(res, [0,0,0]);
}
  
{
    let num = 1;
    let len = 1025;
    let res = Array.from({length : len}, () => num);
    assert_equal(res.length, len);
    let flag = true;
    for (let i = 0; i < res.length; ++i) {
      if (res[i] != num) {
        flag = false;
        break;
      }
    }
    assert_equal(flag, true);
}

{
    function example() {
      let res = Array.from(arguments);
      assert_equal(res, [1,2,3])
    }
    example(1, 2, 3);
}
  
{
    let arrayLike = {0:1.1, 1:12, 2:'ss', length: 3}
    let res = Array.from(arrayLike, x => x + x);
    assert_equal(res, [2.2, 24, "ssss"]);
}
  
{
    let res = Array.from({length : 3}, (_, index) => [index * 2]);
    assert_equal(res, [[0],[2],[4]]);
}

{
    const nonConstructor = {}
    let res = Array.from.call(nonConstructor, {length : 3}, (_, index) => [index * 2]);
    assert_equal(res, [[0],[2],[4]]);
}

// array.from by JSArray
{
    const nonConstructor = {}
    let num = 1
    let len = 1025 // may transfer to dictionary elements type
    let myArray = new Array(1025).fill(num)
    let res = Array.from.call(nonConstructor, myArray);
    assert_equal(res.length, len);
    let flag = true;
    for (let i = 0; i < res.length; ++i) {
      if (res[i] != num || res.at(i) != num) {
        flag = false;
        break;
      }
    }
    assert_equal(flag, true);
}
  
{
    const nonConstructor = {}
    let myArray = new Array(1,2,3,4,5)
    let res = Array.from.call(nonConstructor, myArray);
    assert_equal(res, [1,2,3,4,5]);
}
  
{
    let res = Array.from([1,2,3,4,5]);
    assert_equal(res, [1,2,3,4,5]);
}

// test for String with mapFunc
{
    let str = 'a'.repeat(10)
    let res = Array.from(str, x => x + 's');
    assert_equal(res, ["as","as","as","as","as","as","as","as","as","as"]);
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
    assert_equal(res.length, len);
    assert_equal(flag, true);
}
  
// test for Set with mapFunc
{
    let set = new Set(['test', 'for', 'array', 'from', 'set'])
    let res = Array.from(set, x => x);
    assert_equal(res, ["test","for","array","from","set"]);
}
  
// test for Map with mapFunc
{
    let map = new Map([[1, 'test'], [2, 'for'], [3, 'array'], [4, 'from'], [5, 'map']]);
    let res = Array.from(map, x => x);
    assert_equal(res, [[1,"test"],[2,"for"],[3,"array"],[4,"from"],[5,"map"]]);
}
  
// test for TypedArray with mapFunc
{
    let mapFunc = x => x + x;
    let uint8Array = new Uint8Array([1, 2, 3, 4, 5, 6]);
    let res = Array.from(uint8Array, mapFunc);
    assert_equal(res, [2,4,6,8,10,12]);
}
  
{
    let mapFunc = x => x + x;
    let uint16Array = new Uint16Array([1, 2, 3, 4, 5, 6]);
    let res = Array.from(uint16Array, mapFunc);
    assert_equal(res, [2,4,6,8,10,12]);
}
  
{
    let mapFunc = x => x + x;
    let uint32Array = new Uint32Array([1, 2, 3, 4, 5, 6]);
    let res = Array.from(uint32Array, mapFunc);
    assert_equal(res, [2,4,6,8,10,12]);
}
  
{
    let mapFunc = x => x + x;
    let float32Array = new Float32Array([1, 2, 3, 4, 5, 6]);
    let res = Array.from(float32Array, mapFunc);
    assert_equal(res, [2,4,6,8,10,12]);
}
  
{
    let mapFunc = x => x + x;
    let float64Array = new Float64Array([1, 2, 3, 4, 5, 6]);
    let res = Array.from(float64Array, mapFunc);
    assert_equal(res, [2,4,6,8,10,12]);
}

// Test StringToListResultCache
{
    let str = "foo,bar,baz";
    let res = Array.from(str);
    let resCache = Array.from(str);
    assert_equal(resCache, ["f","o","o",",","b","a","r",",","b","a","z"])
}

function test() {
    var set = [];
    var a = [];
    var p = new Proxy(a, {
        set: function (o, k, v) {
            set.push(k);
            o[k] = v;
            return true;
        }
    });
    Array.from.call(function () { return p; }, { length: 2, 0: 1, 1: 2 });
    return set + "" === "length";
}

assert_equal(test(), true);

test_end();
