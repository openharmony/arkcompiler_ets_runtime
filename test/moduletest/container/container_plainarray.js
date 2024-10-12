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
    let ret = proxy.add(0, "0")
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
    map.set("test plainarray 'add' ret:", ret === undefined)
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

    newPlainArray = proxy.clone()
    res = proxy.removeRangeFrom(1, 2)
    testArray.splice(1, 2)
    if (res > 0) {
        res = newPlainArray.removeRangeFrom(0, 100)
        if (res > 0) {
            res = newPlainArray.isEmpty()
        }
    }
    map.set("test plainarray removeRangeFrom:", res)

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
    ret = proxy1.clear()
    map.set("test plainarray clear:", proxy1.length === 0)
    map.set("test plainarray 'clear' ret:", ret === undefined)
    map.set("test plainarray isEmpty:", proxy1.isEmpty())
    proxy1.add(0, "0")
    proxy1.add(1, "1")
    proxy1.add(2, "2")
    proxy1.add(3, "3")
    proxy1.add(4, "4")
    proxy1.add(5, "5")
    map.set("test plainarray toString:", proxy1.toString() == "0:0,1:1,2:2,3:3,4:4,5:5");
    let empty_pa = new PlainArray();
    try {
        empty_pa.getValueAt(0);
    } catch(err) {
        res = (err =="BusinessError: Container is empty")
        map.set("test getValueAt exception when arraylist is empty:", res)
    }
    try {
        empty_pa.removeRangeFrom(0, 1);
    } catch(err) {
        res = (err =="BusinessError: Container is empty")
        map.set("test removeRangeFrom exception when arraylist is empty:", res)
    }
    try {
        empty_pa.setValueAt(0);
    } catch(err) {
        res = (err =="BusinessError: Container is empty")
        map.set("test setValueAt exception when arraylist is empty:", res)
    }

    try {
        let myPlainArray = new PlainArray();
        myPlainArray.add(1);
        myPlainArray[2147483648];
    } catch(err) {
        let overFlowTest = (err == "BusinessError: The type of \"index\" must be small integer.");
        map.set("test PlainArray[i] overFlowTest:", overFlowTest);
    }

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

    // Math.floor as index input should not throw exception.
    let myPa = new PlainArray();
    myPa.add(0, "a");
    myPa.add(Math.floor(1.5), "b");
    myPa.has(Math.floor(1.5));
    myPa.get(Math.floor(1.5));
    myPa.getValueAt(Math.floor(1.5));
    myPa.setValueAt(Math.floor(1.5), "c");
    myPa.getKeyAt(Math.floor(1.5));
    myPa.getIndexOfKey(Math.floor(1.5));
    myPa.removeAt(Math.floor(1.5));

    if (!res) {
        print("Test PlainArray success!!!");
    } else {
        print("Test PlainArray fail: " + res);
    }
    const v6 = new PlainArray()
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
}
export let plainarrayRes = "Test PlainArray done";
