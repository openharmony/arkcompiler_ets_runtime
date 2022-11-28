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
 * @tc.require: issueI5NO8G
 */
var arrayList = undefined;
if (globalThis["ArkPrivate"] != undefined) {
    arrayList = ArkPrivate.Load(ArkPrivate.ArrayList);
    let arr = new arrayList();
    arr.add(1);
    arr.add(2);

    let map = new Map();
    let flag1 = false;
    try {
        arr["aa"] = 3;
    } catch (e) {
        flag1 = true;
    }
    map.set("flag1", flag1);

    let flag2 = true;
    for (let i = 0; i < arr.length; i++) {
        if (arr[i] != (i + 1)) {
            flag2 = false;
            break;
        }
    }
    map.set("flag2", flag2);

    let arr1 = new arrayList();
    let proxy = new Proxy(arr1, {});
    let testArray = []
    let res = true
    for(let i = 0; i < 10; i++) {
        proxy.add(i)
        testArray.push(i)
    }

    for(let i = 0; i < testArray.length; i++) {
        if (proxy[i] !== testArray[i]) {
            res = false
        }
    }
    map.set("test arraylist add:", res)

    map.set("test arraylist has:", proxy.has(2))
    map.set("test arraylist getCapacity:", proxy.getCapacity() === 15)
    map.set("test arraylist getLastIndexOf:", proxy.getLastIndexOf(1) === 1)
    map.set("test arraylist getIndexOf:", proxy.getIndexOf(5) === 5)

    let array = proxy.convertToArray()
    res = true
    for(let i = 0; i < testArray.length; i++) {
        if (array[i] !== testArray[i]) {
            res = false
        }
    }

    map.set("test arraylist convertToArray:", res)

    let newArrayList = proxy.clone()
    res = true
    for(let i = 0; i < testArray.length; i++) {
        if (newArrayList[i] !== testArray[i]) {
            res = false
        }
    }
    map.set("test arraylist clone:", res)

    proxy.insert(999, 3)
    testArray.splice(3, 0, 999)
    res = true
    for(let i = 0; i < testArray.length; i++) {
        if (proxy[i] !== testArray[i]) {
            res = false
        }
    }
    map.set("test arraylist insert:", res)

    proxy.removeByIndex(9)
    testArray.splice(9, 1)
    res = true
    for(let i = 0; i < testArray.length; i++) {
        if (proxy[i] !== testArray[i]) {
            res = false
        }
    }
    map.set("test arraylist removeByIndex:", res)

    const removeRes = proxy.remove(7)
    testArray.splice(8, 1)
    res = true
    for(let i = 0; i < testArray.length; i++) {
        if (proxy[i] !== testArray[i]) {
            res = false
        }
    }
    map.set("test arraylist remove:", res)
    map.set("test arraylist removeRes:", removeRes)

    proxy.removeByRange(1, 3)
    testArray.splice(1, 2)
    res = true
    for(let i = 0; i < testArray.length; i++) {
        if (proxy[i] !== testArray[i]) {
            res = false
        }
    }
    map.set("test arraylist removeByRange:", res)

    res = true
    proxy.forEach((value, index) => {
        if (value !== testArray[index]) {
            res = false
        }
    })

    map.set("test arraylist forEach:", res)

    res = true
    let subArrayList = proxy.subArrayList(1, 3)
    const newtestArray = testArray.slice(1, 3)
    for(let i = 0; i < subArrayList.length; i++) {
        if (newtestArray[i] !== subArrayList[i]) {
            res =  false
        }
    }
    map.set("test arraylist subArrayList:", res)

    res = true
    let j = 0
    for (const data of proxy) {
      if (data !== testArray[j]) {
        res = false
      }
      j++;
    }
    map.set("test arraylist for of:", res)

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
    map.set("test arraylist Symbol.iterator:", res)

    proxy.replaceAllElements((item, index) => {
        return item * 2
    })
    res = true
    for(let i = 0; i < testArray.length; i++) {
        if (proxy[i] !== testArray[i] * 2) {
            res = false
        }
    }
    map.set("test arraylist replaceAllElements:", res)

    let arr2 = new arrayList();
    let proxy1 = new Proxy(arr2, {});
    proxy1.add(4);
    proxy1.add(3);
    proxy1.add(1);
    proxy1.add(2);
    proxy1.add(0);
    proxy1.sort((a,b) => a-b);
    res = true
    for (let i = 0; i < 5; i++) {
        if (proxy1[i] !== i) {
            res = false
        }
    }
    map.set("test arraylist sort:", res)

    proxy1.clear()
    map.set("test arraylist clear:", proxy1.length === 0)
    map.set("test arraylist isEmpty:", proxy1.isEmpty())
    proxy1.add(4);
    proxy1.add(3);
    proxy1.add(1);
    proxy1.add(2);
    proxy1.add(0);
    proxy1.sort((a,b) => a-b);
    res = true
    for (let i = 0; i < 5; i++) {
        if (proxy1[i] !== i) {
            res = false
        }
    }
    map.set("test arraylist clear and add:", res)

    proxy1.insert(50, 0);
    map.set("test arraylist insert index === 0:", proxy1[0] === 50)
    proxy1.increaseCapacityTo(20)
    map.set("test arraylist increaseCapacityTo:", proxy1.getCapacity() === 20)

    proxy1.trimToCurrentLength()
    map.set("test arraylist trimToCurrentLength:", proxy1.getCapacity() === 6)

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
    let de = new arrayList();
    try {
        de.forEach(123);
    } catch(err) {
        if (err.name != "BusinessError") {
            print("ArrayList forEach throw error fail");
        }
    }
    let test1 = new arrayList();
    for (let k = 0; k < 10; k++) {
        test1.add(k);
    }
    var keyName = "";
    for (const key in test1) {
        keyName += key;
    }
    if (keyName != "0123456789") {
        print("ArrayList for in fail")
    }
    if (!flag) {
        print("Test ArrayList success!!!");
    } else {
        print("Test ArrayList fail: " + flag);
    }
}

