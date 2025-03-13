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
import {testdProxyArray1}  from "./utility";

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
    map.set("test deque for of:", res);
    
    testdProxyArray1(proxy, res, testArray);
    map.set("test deque Symbol.iterator:", res)

    map.set("test deque has:",  proxy.has(7))
    map.set("test deque popFirst:",  proxy.getFirst() === 0)
    map.set("test deque popLast:",  proxy.getLast() === 19)
    map.set("test deque popFirst:",  proxy.popFirst() === 0)
    map.set("test deque popLast:",  proxy.popLast() === 19)

    try {
        let myDeque = new Deque();
        myDeque.insertEnd(1);
        myDeque[2147483648];
    } catch(err) {
        let overFlowTest = (err == "BusinessError: The type of \"index\" must be small integer.");
        map.set("test Deque[i] overFlowTest:", overFlowTest);
    }

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
    const v1 = new Deque();
    const v2 = [Deque,Deque];
    class C4{
    ["100"] = v2
    }
    const v5 = new C4();
    Object.assign(v1,v5)
    print(JSON.stringify(v1))
    const v6 = new Deque()
    try {
        v6[4294967295]
    } catch (error) {
        print(error)
    }
    let v22;
    class C11 extends Deque {

    }
    v22 = new C11()
    v22[2295334561] = 0
    print(v22.length)
    class C44 extends Deque{
        constructor(a) {
            super()
            super[a] = 11
            const o10 = {
                ...this
            }
        }
    }
    new C44(38062)

    /*
     * @tc.name: checkcontainerloadhclass
     * @tc.desc: Test ContainersDequeStubBuilder::ForEach whether judge isHeapObject
     *           before loading hClass.
     * @tc.type: FUNC
     * @tc.require: issueIBQ709
     */
    {
        let v1 = new Deque(0);
        let arr1 = [1];
        try {
            Reflect.apply(v1.forEach, 123, arr1);
        } catch (e) {
            print(e);
        }
    }
}
export let dequeRes = "Test Deque done";
