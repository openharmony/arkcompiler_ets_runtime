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
var Stack = undefined;
if (globalThis["ArkPrivate"] != undefined) {
    Stack = ArkPrivate.Load(ArkPrivate.Stack);
    let stack = new Stack();
    let proxy = new Proxy(stack, {});
    let res = true
    let testArray = []
    let map = new Map();
    for(let i = 0; i < 10; i++) {
        proxy.push(i)
        testArray.push(i)
    }

    res = true
    for(let i = 0; i < testArray.length; i++) {
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
    map.set("test stack for of:", res)

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
    map.set("test stack Symbol.iterator:", res)

    map.set("test stack peek:",  proxy.peek() === 9)
    map.set("test stack locate:",  proxy.locate(5) === 5)

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
    if (!flag) {
        print("Test Stack success!!!");
    } else {
        print("Test Stack fail: " + flag);
    }
}