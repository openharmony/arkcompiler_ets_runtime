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

var Queue = undefined;
if (globalThis["ArkPrivate"] != undefined) {
    Queue = ArkPrivate.Load(ArkPrivate.Queue);
    let v4 = new Queue();
    v4[3829299154] = Queue;
    print(v4.length);
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
    testdProxyArray1(proxy, res, testArray);

    map.set("test queue Symbol.iterator:", res)

    map.set("test queue popFirst:",  proxy.getFirst() === 0)
    map.set("test queue pop:",  proxy.pop() === 0)

    try {
        let myQueue = new Queue();
        myQueue.add(1);
        myQueue[2147483648];
    } catch(err) {
        let overFlowTest = (err == "BusinessError: The type of \"index\" must be small integer.");
        map.set("test Queue[i] overFlowTest:", overFlowTest);
    }

    {
        let queue = new Queue();
        let result = queue.add(1);
        map.set("test Queue add0001:", result == true);
    }

    {
        let queue = new Queue();
        let result = queue.add("a");
        map.set("test Queue add0002:", result == true);
    }

    {
        let queue = new Queue();
        let a = {
          name: "Dylon", age: "13"
        };
        let result = queue.add(a);
        map.set("test Queue add0003:", result == true);
    }

    {
        let queue = new Queue();
        let a = {
          name: "Dylon", age: "13"
        };
        let b = 'a'
        let c = 1
        let result1 = queue.add(a)
        let result2 = queue.add(b)
        let result3 = queue.add(c)
        let myFlag = true;
        if (result1 != true || result2 != true || result3 != true) {
            myFlag = false;
        }
        map.set("test Queue add0004:", myFlag);
    }

    {
        let queue = new Queue();
        let result = queue.add(undefined)
        map.set("test Queue add0005:", result == true);
    }

    {
        let queue = new Queue();
        let a = {
          name: "Dylon", age: "13"
        };
        let b = 'a'
        let c = 1
        queue.add(a)
        queue.add(b)
        queue.add(c)
        let result = queue.pop();
        map.set("test Queue pop0001:", result == a);
    }

    {
        let queue = new Queue();
        let result = queue.pop();
        map.set("test Queue pop0002:", result == undefined);
    }

    {
        let queue = new Queue();
        let a = {
          name: "Dylon", age: "13"
        };
        let b = 'a'
        let c = 1
        queue.add(a)
        queue.add(b)
        queue.add(c)
        let result = queue.getFirst();
        map.set("test Queue getFirst0001:", result == a);
    }

    {
        let queue = new Queue();
        let result = queue.getFirst();
        map.set("test Queue getFirst0002:", result == undefined);
    }

    {
        let queue = new Queue();
        let a = {
          name: "Dylon", age: "13"
        };
        let b = 'a';
        let c = 1;
        queue.add(a);
        queue.add(b);
        queue.add(c);
        let myFlag = true;
        queue.forEach((value, index) => {
          let result = value;
          let result1 = index;
          if (result != value || result1 != index) {
            myFlag = false;
          }
          map.set("test Queue forEach0001:", myFlag);
        });
    }

    {
        let queue = new Queue();
        let a = {
          name: "Dylon", age: "13"
        };
        let b = 'a';
        let c = 1;
        queue.add(a);
        queue.add(b);
        queue.add(c);
        let iter = queue[Symbol.iterator]();
        let temp = iter.next().value;
        let myFlag = true;
        while (temp != undefined) {
          temp = iter.next().value;
          if (temp != temp) {
            myFlag = false;
          }
        }
        map.set("test Queue [Symbol.iterator]0001:", myFlag);
    }

    {
        let queue = new Queue();
        let a = {
          name: "Dylon", age: "13"
        };
        let b = 'a';
        let c = 1;
        queue.add(a);
        queue.add(b);
        queue.add(c);
        let myFlag = true;
        queue.forEach(() => {
            if (queue.length != 3) {
                myFlag = false;
            }
        });
        map.set("test Queue forEach0002:", myFlag);
    }

    {
        let queue = new Queue();
        let a = {
          name: "Dylon", age: "13"
        };
        queue.add(a);
        let myFlag = true;
        queue.forEach((value) => {
            if (queue.getFirst() != value) {
                myFlag = false;
            }
        });
        map.set("test Queue forEach0003:", myFlag);
    }

    {
        let queue = new Queue();
        let a = {
          name: "Dylon", age: "13"
        };
        queue.add(a);
        let myFlag = true;
        queue.forEach((value, index, queue) => {
            if (queue.getFirst() != value) {
                myFlag = false;
            }
        });
        map.set("test Queue forEach0004:", myFlag);
    }

    {
        let queue = new Queue();
        queue.add(1);
        let myFlag = true;
        queue.forEach((value, index, queue) => {
            if (queue.getFirst() != value) {
                myFlag = false;
            }
        }, queue);
        if (queue.getFirst() != 1) {
            myFlag = false;
        }
        map.set("test Queue forEach0005:", myFlag);
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

    let size_test = new Queue();
    for (var i = 0; i < 6; i++) {
        size_test.add(i);
    }
    print(size_test.length);

    const v1 = [Queue,Queue,Queue,Queue];
    const v2 = new Queue();
    const v3 = [-63084,3,536870889,-1926,29182,-4294967296,1222477883];
    const v5 = [v1];
    try {
        Reflect.apply(v3.unshift, v2, v5);
    } catch(error) {
        print(error);
    }
    const v6 = new Queue()
    function f2(a3) {
        return a3
    }
    const o5 = {
        "get" : f2,
    }
    const v7 = new Proxy(v6, o5)
    try {
        v7[1073741823]
    } catch (error) {
        print(error)
    }
    class C4 extends Queue{
        constructor(a) {
            super()
            super[a] = 11
            const o10 = {
                ...this
            }
        }
    }
    new C4(38062)

    /*
     * @tc.name: checkcontainerloadhclass
     * @tc.desc: Test ContainersQueueStubBuilder::ForEach whether judge isHeapObject
     *           before loading hClass.
     * @tc.type: FUNC
     * @tc.require: issueIBQ709
     */
    {
        let v1 = new Queue(0);
        let arr1 = [1];
        try {
            Reflect.apply(v1.forEach, 123, arr1);
        } catch (e) {
            print(e);
        }
    }
}
export let queueRes = "Test Queue done";
