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
var PlainArray = undefined;
if (globalThis["ArkPrivate"] != undefined) {
    PlainArray = ArkPrivate.Load(ArkPrivate.PlainArray);

    let map = new Map();
    let plainArray = new PlainArray();
    let proxy = new Proxy(plainArray, {});
    let testArray = ["0", "1", "2", "3", "4", "5"]
    let res = true
    proxy.add(0, "0")
    proxy.add(1, "1")
    proxy.add(2, "2")
    proxy.add(3, "3")
    proxy.add(4, "4")
    proxy.add(5, "5")

    for(let i = 0; i < testArray.length; i++) {
        if (proxy[i] !== testArray[i]) {
            res = false
        }
    }
    map.set("test plainarray add:", res)
    map.set("test plainarray length:", proxy.length === 6)
    map.set("test plainarray has:", proxy.has(2))
    map.set("test plainarray getIndexOfValue:", proxy.getIndexOfValue("1") === 1)
    map.set("test plainarray getIndexOfKey:", proxy.getIndexOfKey(5) === 5)
    map.set("test plainarray getKeyAt:", proxy.getKeyAt(1) === 1)
    map.set("test plainarray getValueAt:", proxy.getValueAt(2) === "2")
    
    let newPlainArray = proxy.clone()
    res = true
    for(let i = 0; i < testArray.length; i++) {
        if (newPlainArray[i] !== testArray[i]) {
            res = false
        }
    }
    map.set("test plainarray clone:", res)

    const removeRes = proxy.remove(3)
    testArray.splice(3, 1)
    map.set("test plainarray removeRes:", removeRes)

    proxy.removeAt(2)
    testArray.splice(2, 1)
    res = true
    for(let i = 0; i < testArray.length; i++) {
        if (proxy.get(i) !== testArray[i]) {
            res = false
        }
    }

    let removeFrom = proxy.removeRangeFrom(1, 2)
    testArray.splice(1, 2)

    map.set("test plainarray removeRangeFrom:", removeFrom)

    res = true
    proxy.forEach((i, d) => {
    })

    map.set("test plainarray forEach:", res)

    res = true
    let testArray3 = [0, 5]
    let j = 0
    for (const data of proxy) {
    }
    map.set("test plainarray for of:", res)

    res = true
    let itr = proxy[Symbol.iterator]();
    let tmp = undefined;
    let testArray1 = []
    do {
      tmp = itr.next().value;
      testArray1.push(tmp);
    } while (tmp != undefined);
    map.set("test plainarray Symbol.iterator:", res)

    let arr2 = new PlainArray();
    let proxy1 = new Proxy(arr2, {});
    proxy1.add(0, "0")
    proxy1.add(1, "1")
    proxy1.add(2, "2")
    proxy1.add(3, "3")
    proxy1.add(4, "4")
    proxy1.add(5, "5")
    proxy1.setValueAt(2, "123")
    map.set("test plainarray setValueAt and get:", proxy1.get(2) === "123")
    proxy1.clear()
    map.set("test plainarray clear:", proxy1.length === 0)
    map.set("test plainarray isEmpty:", proxy1.isEmpty())
    proxy1.add(0, "0")
    proxy1.add(1, "1")
    proxy1.add(2, "2")
    proxy1.add(3, "3")
    proxy1.add(4, "4")
    proxy1.add(5, "5")
    map.set("test plainarray toString:", proxy1.toString() == "0:0,1:1,2:2,3:3,4:4,5:5");

    res = undefined;
    function elements(value, key, map) {
        if (!value) {
            if (!res) {
                res = [];
            }
            res.push(key);
        }
    }
    map.forEach(elements);

    let de = new PlainArray();
    try {
        de.forEach(123);
    } catch(err) {
        if (err.name != "BusinessError") {
            print("PlainArray forEach throw error fail");
        }
    }
    if (!res) {
        print("Test PlainArray success!!!");
    } else {
        print("Test PlainArray fail: " + res);
    }
}
