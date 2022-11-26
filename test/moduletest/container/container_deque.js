/*
 * Copyright (c) 2022 Huawei Device Co., Ltd.
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
 * @tc.name:container
 * @tc.desc:test container
 * @tc.type: FUNC
 * @tc.require: 
 */
var Deque = undefined;
if (globalThis["ArkPrivate"] != undefined) {
    Deque = ArkPrivate.Load(ArkPrivate.Deque);
    let deque = new Deque();
    let proxy = new Proxy(deque, {});
    let res = true
    let testArray = []
    let map = new Map();
    for(let i = 0; i < 10; i++) {
        testArray.push(i)
    }
    for(let i = 9; i >= 0; --i) {
        proxy.insertFront(i)
    }

    res = true
    for(let i = 0; i < testArray.length; i++) {
        if (proxy[i] !== testArray[i]) {
            res = false
        }
    }
    map.set("test deque insertFront:", res)

    for(let i = 10; i < 20; i++) {
        proxy.insertEnd(i)
        testArray.push(i)
    }

    res = true
    for(let i = 0; i < testArray.length; i++) {
        if (proxy[i] !== testArray[i]) {
            res = false
        }
    }
    map.set("test deque insertEnd:", res)

    res = true
    proxy.forEach((i, d) => {
        if (d !== testArray[i]) {
            res = false
        }
    })

    map.set("test deque forEach:", res)

    res = true
    let j = 0
    for (const data of proxy) {
      if (data !== testArray[j]) {
        res = false
      }
      j++;
    }
    map.set("test deque for of:", res)

    let itr = proxy[Symbol.iterator]();
    let tmp = undefined;
    let testArray1 = []
    do {
      tmp = itr.next().value;
      testArray1.push(tmp);
    } while (tmp != undefined);

    for (let k = 0; k < proxy.length; k++) {
      if (testArray1[k] !== testArray[k]) {
        res = false
      }
    }
    map.set("test deque Symbol.iterator:", res)

    map.set("test deque has:",  proxy.has(7))
    map.set("test deque popFirst:",  proxy.getFirst() === 0)
    map.set("test deque popLast:",  proxy.getLast() === 19)
    map.set("test deque popFirst:",  proxy.popFirst() === 0)
    map.set("test deque popLast:",  proxy.popLast() === 19)

    let flag = undefined;
    function elements(value, key, map) {
        if (!value) {
            if (!flag) {
                flag = [];
            }
            flag.push(key);
        }
    }
    map.forEach(elements);
    var arr1 = [];
    let myTest = new Deque();
    for (let i = 0; i < 10; i++) {
        myTest.insertEnd(i);
    }
    for (let i = 0; i < 5; i++) {
        myTest.popFirst();
    }
    myTest.forEach(
        function myFunc(item, index, arr) {
            arr1.push(item);
        }
    );
    for (let j = 5; j < 10; j++) {
        if (arr1[j - 5] != j) {
            print("Deque forEach first argv fail");
        }
    }

    let de = new Deque();
    try {
        de.forEach(123);
    } catch(err) {
        if (err.name != "BusinessError") {
            print("Deque forEach throw error fail");
        }
    }
    let test1 = new Deque();
    for (let k = 0; k < 10; k++) {
        test1.insertEnd(k);
    }
    var keyName = "";
    for (const key in test1) {
        keyName += key;
    }
    if (keyName != "0123456789") {
        print("Deque for in fail")
    }
    if (!flag) {
        print("Test Deque success!!!");
    } else {
        print("Test Deque fail: " + flag);
    }
}