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
import {testdProxyArray1}  from "./utility";

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

    let isClear = false;
    let tempArr = new arrayList();
    tempArr.add(1);
    tempArr.add(2);
    tempArr.add(3);
    tempArr.sort((a, b) => {
        if (!isClear) {
            tempArr.clear();
            isClear = true;
        }
        return a - b;
    });
    map.set("test arraylist sort callbackFN:", tempArr.length === 3);

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

    map.set("test arraylist forEach:", res);

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
    map.set("test arraylist for of:", res);

    testdProxyArray1(proxy, res, testArray);
    
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

    let testlist = new arrayList();
    try {
        testlist.removeByIndex(0);
    } catch(err) {
        res = (err =="BusinessError: Container is empty")
        map.set("test RemoveByIndex exception when arraylist is empty:", res)
    }
    try {
        testlist.removeByRange(0, 1);
    } catch(err) {
        res = (err =="BusinessError: Container is empty")
        map.set("test RemoveByRange exception when arraylist is empty:", res)
    }
    try {
        testlist.subArrayList(0, 1);
    } catch(err) {
        res = (err =="BusinessError: Container is empty")
        map.set("test SubArrayList exception when arraylist is empty:", res)
    }
    testlist.add(7);
    testlist.add(9);
    map.set("test arraylist JSAPIArrayList::GetProperty:", testlist[Math.floor(1)] === 9);

    try {
        let myArrayList = new arrayList();
        myArrayList.add(1);
        myArrayList[2147483648];
    } catch(err) {
        let overFlowTest = (err == "BusinessError: The type of \"index\" must be small integer.");
        map.set("test ArrayList[i] overFlowTest:", overFlowTest);
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

    // Math.foor as index input should not have exception.
    let myAl = new arrayList();
    myAl.add(1);
    myAl.add(2);
    myAl.add(3);
    myAl.insert(30, Math.floor(1.3));
    myAl.removeByIndex(Math.floor(1.3));
    
    const v1 = new arrayList()
    v1.add(1)
    v1.add(2)
    v1.add(3)
    let oldLen = v1.length
    v1.replaceAllElements((val,index,temp)=>{
        temp.add(val)
    })
    print(oldLen * 2 === v1.length)
    const v3 = new arrayList();
    const v4 = [arrayList,arrayList];
    class C4{
    ["100"] = v4
    }
    const v5 = new C4();
    try {
        Object.assign(v3,v5)  
    } catch (error) {
        print(error)
    }
    
    const v11 = new arrayList()
    function f2(a3, a4) {
        const o5 = {}
        return o5;
    }
    const o6 = {
        "set" : f2
    }
    try {
        const v8Proxy = new Proxy(v11,o6)
        v8Proxy[4] = "no"
    } catch (error) {
        print(error)
    }

    if (!flag) {
        print("Test ArrayList success!!!");
    } else {
        print("Test ArrayList fail: " + flag);
    }
}
export let arraylistRes = "Test ArrayList";
