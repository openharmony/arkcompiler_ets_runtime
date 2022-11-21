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
var List = undefined;
if (globalThis["ArkPrivate"] != undefined) {
    List = ArkPrivate.Load(ArkPrivate.List);
    let list = new List();
    const testArray = []
    let map = new Map();
    map.set("test list isEmpty1:", list.isEmpty() === true)
    for(let i = 0; i < 10; i++) {
        list.add(i)
        testArray.push(i)
    }

    map.set("test list isEmpty2:", list.isEmpty() === false)
    map.set("test list get 1:", list.get(1) === 1)
    map.set("test list has:",  list.has(8))
    map.set("test list not has:", list.has(123) === false)

    let list1 = new List();
    const testArray2 = []
    for(let i = 0; i < 10; i++) {
        list1.add(i)
        testArray2.push(i)
    }

    map.set("test list equal:", list.equal(list1))
    list.add(10)
    testArray.push(10)
    map.set("test list equal:", list.equal(list1) === false)
    map.set("test list getLastIndexOf:", list.getLastIndexOf(1) === 1)
    map.set("test list getIndexOf:", list.getIndexOf(5) === 5)

    list.removeByIndex(10)
    testArray.splice(10, 1)
    let res = true
    for(let i = 0; i < testArray.length; i++) {
        if (list[i] !== testArray[i]) {
            res = false
        }
    }
    map.set("test list removeByIndex:", res)

    list.remove(9)
    testArray.splice(9, 1)
    res = true
    for(let i = 0; i < testArray.length; i++) {
        if (list[i] !== testArray[i]) {
            res = false
        }
        testArray[i] = testArray[i] * 2
    }
    map.set("test list remove:", res)

    list.replaceAllElements((item, index) => {
        return item * 2
    })
    res = true
    for(let i = 0; i < testArray.length; i++) {
        if (list[i] !== testArray[i] * 2) {
            res = false
        }
    }
    map.set("test list replaceAllElements:", res)
    map.set("test list getFirst:", list.getFirst() === 0)
    map.set("test list getLast:", list.getLast() === 16)
    list.insert(999, 3)
    testArray.splice(3, 0, 999)
    res = true
    for(let i = 0; i < testArray.length; i++) {
        if (list[i] !== testArray[i]) {
            res = false
        }
    }
    map.set("test list insert:", res)

    list.set(5, 888)
    testArray[5] = 888
    res = true
    for(let i = 0; i < testArray.length; i++) {
        if (list[i] !== testArray[i]) {
            res = false
        }
    }
    map.set("test list set:", res)

    let list2 = new List();
    list2.add(4);
    list2.add(3);
    list2.add(1);
    list2.add(2);
    list2.add(0);
    list2.sort((a,b) => a-b);
    res = true
    for (let i = 0; i < 5; i++) {
        if (list2[i] !== i) {
            res = false
        }
    }
    map.set("test list sort:", res)

    res = true
    let subList = list.getSubList(1, 3)
    const newtestArray = testArray.slice(1, 3)
    for(let i = 0; i < subList.length; i++) {
        if (newtestArray[i] !== subList[i]) {
            res =  false
        }
    }
    map.set("test list getSubList:", res)

    res = true
    const arr = list.convertToArray()
    for (let i = 0; i < arr.length; i++) {
        if (arr[i] !== testArray[i]) {
            res = false
        }
    }
    map.set("test list convertToArray:", res)

    res = true
    let i = 0
    for (const data of list) {
        if (data !== testArray[i]) {
            res = false
        }
        i++;
    }
    map.set("test list itertor:", res)

    res = true
    list1.forEach((i, d) => {
        if (d !== testArray2[i]) {
            res = false
        }
    })
    map.set("test list forEach:", res)
    list2.clear()
    map.set("test list clear:", list2.length === 0)
    res = true
    const testArrayClear = []
    for(let i = 0; i < 10; i++) {
        list2.add(i)
        testArrayClear.push(i)
    }
    for(let i = 0; i < testArrayClear.length; i++) {
        if (list2[i] !== testArrayClear[i]) {
            res = false
        }
    }
    map.set("test list clear and add:", res)
    map.set("test list get:", list1.get(200) === undefined)
    map.set("test list getLastIndexOf:", list1.getLastIndexOf('abc') === -1)
    let flag = false;
    try {
        list1.removeByIndex(99)
    } catch (error) {
        flag = true;
    }
    map.set("test list removeByIndex:", flag)
    res = list1.remove(888)
    map.set("test list remove:", !res)

    let list3 = new List();
    let proxy = new Proxy(list3, {});
    map.set("test list proxy isEmpty1:", proxy.isEmpty() === true);
    const testArray3 = []
    for(let i = 0; i < 10; i++) {
        proxy.add(i)
        testArray3.push(i)
    }

    map.set("test list proxy isEmpty2:", proxy.isEmpty() === false);
    map.set("test list get 1:", proxy.get(1) === 1)
    map.set("test list has:",  proxy.has(8))
    map.set("test list not has:", proxy.has(123) === false)

    let list4 = new List();
    let proxy1 = new Proxy(list4, {});
    const testArray4 = []
    for(let i = 0; i < 10; i++) {
        proxy1.add(i)
        testArray4.push(i)
    }

    proxy.add(10)
    testArray3.push(10)
    map.set("test list equal:", proxy.equal(proxy1) === false)
    map.set("test list getLastIndexOf:", proxy.getLastIndexOf(1) === 1)
    map.set("test list getIndexOf:", proxy.getIndexOf(5) === 5)

    proxy.removeByIndex(10)
    testArray3.splice(10, 1)
    res = true
    for(let i = 0; i < testArray3.length; i++) {
        if (proxy[i] !== testArray3[i]) {
            res = false
        }
    }
    map.set("test list removeByIndex:", res)

    proxy.remove(9)
    testArray3.splice(9, 1)
    res = true
    for(let i = 0; i < testArray3.length; i++) {
        if (proxy[i] !== testArray3[i]) {
            res = false
        }
        testArray3[i] = testArray3[i] * 2
    }
    map.set("test list remove:", res)

    proxy.replaceAllElements((item, index) => {
        return item * 2
    })
    res = true
    for(let i = 0; i < testArray3.length; i++) {
        if (proxy[i] !== testArray3[i]) {
            res = false
        }
    }
    map.set("test list replaceAllElements:", res)
    map.set("test list getFirst:", proxy.getFirst() === 0)
    map.set("test list getLast:", proxy.getLast() === 16)
    proxy.insert(999, 3)
    testArray3.splice(3, 0, 999)
    res = true
    for(let i = 0; i < testArray3.length; i++) {
        if (proxy[i] !== testArray3[i]) {
            res = false
        }
    }
    map.set("test list insert:", res)

    proxy.set(5, 888)
    testArray3[5] = 888
    res = true
    for(let i = 0; i < testArray3.length; i++) {
        if (proxy[i] !== testArray3[i]) {
            res = false
        }
    }
    map.set("test list set:", res)

    let list5 = new List();
    let proxy2 = new Proxy(list5, {});
    proxy2.add(4);
    proxy2.add(3);
    proxy2.add(1);
    proxy2.add(2);
    proxy2.add(0);
    proxy2.sort((a,b) => a-b);
    res = true
    for (let i = 0; i < 5; i++) {
        if (proxy2[i] !== i) {
            res = false
        }
    }
    map.set("test list sort:", res)

    res = true
    let subList1 = proxy.getSubList(1, 3)
    const newtestArray1 = testArray3.slice(1, 3)
    for(let i = 0; i < subList1.length; i++) {
        if (newtestArray1[i] !== subList1[i]) {
            res =  false
        }
    }
    map.set("test list getSubList:", res)

    res = true
    const arr1 = proxy.convertToArray()
    for (let i = 0; i < arr1.length; i++) {
        if (arr1[i] !== testArray3[i]) {
            res = false
        }
    }
    map.set("test list convertToArray:", res)

    res = true
    let k = 0
    for (const data of proxy1) {
        if (data !== testArray4[k]) {
            res = false
        }
        k++;
    }
    map.set("test list iterator:", res)

    res = true
    proxy1.forEach((i, d) => {
        if (d !== testArray4[i]) {
            res = false
        }
    })
    map.set("test list forEach:", res)
    proxy2.clear()
    map.set("test list clear:", proxy2.length === 0)
    map.set("test list get:", proxy1.get(200) === undefined)
    map.set("test list getLastIndexOf:", proxy1.getLastIndexOf('abc') === -1)
    flag = false;
    try {
        proxy1.removeByIndex(99)
    } catch (error) {
        flag = true;
    }
    map.set("test list removeByIndex:", flag)
    res = proxy1.remove(888)
    map.set("test list remove:", !res)
    flag = undefined;
    function elements(value, key, map) {
        if (!value) {
            if (!flag) {
                flag = [];
            }
            flag.push(key);
        }
    }
    map.forEach(elements);

    let de = new List();
    try {
        de.forEach(123);
    } catch(err) {
        if (err.name != "BusinessError") {
            print("List forEach throw error fail");
        }
    }
    if (!flag) {
        print("Test List success!!!");
    } else {
        print("Test List fail: " + flag);
    }
}
