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
var Queue = undefined;
if (globalThis["ArkPrivate"] != undefined) {
    Queue = ArkPrivate.Load(ArkPrivate.Queue);
    let queue = new Queue();
    let proxy = new Proxy(queue, {});
    let res = true
    let testArray = []
    let map = new Map();
    for(let i = 0; i < 10; i++) {
        proxy.add(i)
        testArray.push(i)
    }

    res = true
    for(let i = 0; i < testArray.length; i++) {
        if (proxy[i] !== testArray[i]) {
            res = false
        }
    }
    map.set("test queue add:", res)

    res = true
    proxy.forEach((i, d) => {
        if (d !== testArray[i]) {
            res = false
        }
    })

    map.set("test queue forEach:", res)

    res = true
    let j = 0
    for (const data of proxy) {
      if (data !== testArray[j]) {
        res = false
      }
      j++;
    }
    map.set("test queue for of:", res)

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
    map.set("test queue Symbol.iterator:", res)

    map.set("test queue popFirst:",  proxy.getFirst() === 0)
    map.set("test queue pop:",  proxy.pop() === 0)

    let flag = undefined;
    function elements(value, key, map) {
        if (!value) {
            if (!flag) {
                flag = [];
            }
            flag.push(key);
        }
    }

    let myTest = new Queue();
    var arr1 = [];
    for (var i = 0; i < 10; i++) {
        myTest.add(i);
    }
    for (var i = 0; i < 5; i++) {
        myTest.pop();
    }
    myTest.forEach(
        function myFunc(item, index, arr) {
            arr1.push(item);
        }
    );
    for (let j = 5; j < 10; j++) {
        if (arr1[j - 5] != j) {
            print("Queue forEach first argv fail");
        }
    }

    let de = new Queue();
    try {
        de.forEach(123);
    } catch(err) {
        if (err.name != "BusinessError") {
            print("Queue forEach throw error fail");
        }
    }
    let test1 = new Queue();
    for (let k = 0; k < 10; k++) {
        test1.add(k);
    }
    var keyName = "";
    for (const key in test1) {
        keyName += key;
    }
    if (keyName != "0123456789") {
        print("Queue for in fail")
    }
    map.forEach(elements);
    if (!flag) {
        print("Test Queue success!!!");
    } else {
        print("Test Queue fail: " + flag);
    }
}