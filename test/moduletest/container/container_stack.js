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

var Stack = undefined;
if (globalThis["ArkPrivate"] != undefined) {
    Stack = ArkPrivate.Load(ArkPrivate.Stack);
    let stack = new Stack();
    let proxy = new Proxy(stack, {});
    let res = true
    let testArray = []
    let map = new Map();

    // test isEmpty true
    map.set("test proxy isEmpty ture:", proxy.isEmpty() == true)

    for (let i = 0; i < 10; i++) {
        proxy.push(i)
        testArray.push(i)
    }

    // test isEmpty false
    map.set("test proxy isEmpty false:", proxy.isEmpty() == false)

    res = true
    for (let i = 0; i < testArray.length; i++) {
        if (proxy[i] !== testArray[i]) {
            res = false
        }
    }
    map.set("test stack add:", res)

    res = true
    proxy.forEach((i, d) => {
        if (d !== testArray[i]) {
            res = false
        }
    })

    map.set("test stack forEach:", res)

    res = true
    let j = 0
    for (const data of proxy) {
        if (data !== testArray[j]) {
            res = false
        }
        j++;
    }
    map.set("test stack for of:", res);    
    testdProxyArray1(proxy, res, testArray);
    map.set("test stack Symbol.iterator:", res)

    map.set("test stack peek:", proxy.peek() === 9)
    map.set("test stack locate:", proxy.locate(5) === 5)

    // test proxy pop
    let popStack = new Stack();
    let popProxy = new Proxy(popStack, {});
    for (let i = 0; i < 10; i++) {
        popProxy.push(i);
    }
    for (let i = 9; i >= 0; i--) {
        map.set("test proxy stack pop:" + i, popProxy.pop() == i);
    }

    try {
        let myStack = new Stack();
        myStack.push(1);
        myStack[2147483648];
    } catch(err) {
        let overFlowTest = (err == "BusinessError: The type of \"index\" must be small integer.");
        map.set("test Stack[i] overFlowTest:", overFlowTest);
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

    let de = new Stack();
    try {
        de.forEach(123);
    } catch (err) {
        if (err.name != "BusinessError") {
            print("Stack forEach throw error fail");
        }
    }
    let test1 = new Stack();
    for (let k = 0; k < 10; k++) {
        test1.push(k);
    }
    var keyName = "";
    for (const key in test1) {
        keyName += key;
    }
    if (keyName != "0123456789") {
        print("Stack for in fail")
    }
    try {
        class MyStack extends Stack{
            100
        }
        new MyStack();
        print("Test MyStack extends Stack failed");
    } catch (error) {
    }
    if (!flag) {
        print("Test Stack success!!!");
    } else {
        print("Test Stack fail: " + flag);
    }
    const v5 = new Stack()
    try {
        v5[2147483649]
    } catch (error) {
        print(error)
    }
}
export let stackRes = "Test Stack done";
