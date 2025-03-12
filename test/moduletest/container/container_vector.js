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
var FastVector = undefined;
if (globalThis["ArkPrivate"] != undefined) {
    FastVector = ArkPrivate.Load(ArkPrivate.Vector);

    let map = new Map();
    let vector = new FastVector();
    vector.add(4); // index is 0
    vector.add(3);
    vector.add(1);
    vector.add(5);
    vector.add(14);
    let res = vector.toString();
    map.set("test add and toString:", res);
    // test insert, length, get, getIndexOf
    vector.insert(2, 2);
    map.set("test length:", vector.length == 6);
    map.set("test get(index is 2):", vector.get(2) == 2);
    map.set("test get(index is 3):", vector.get(3) !== 3); // false
    map.set("test getIndexOf(target is 3):", vector.getIndexOf(3) == 1); // true
    map.set("test getIndexOf(target is 2):", vector.getIndexOf(2) !== 5); // false
    // test isEmpty
    map.set("test isEmpty:", !vector.isEmpty());

    let vec = vector.clone();
    // test clear
    vector.clear();
    map.set("test clear:", vector.isEmpty());
    // // test set, clone
    vec.set(2, 8);
    map.set("test set:", vec.get(2) == 8 && vec.length == 6);
    // trimToCurrentLength
    vec.trimToCurrentLength();
    map.set("test trimToCurrentLength1:", vec.getCapacity() === 6);
    vec.trimToCurrentLength();
    map.set("test trimToCurrentLength2:", vec.getCapacity() === 6);
    // test subvector
    let subVec = vec.subVector(0, 3);
    map.set("test subVector and tostring:", subVec.toString());
    // test replaceAllElements
    subVec.replaceAllElements((item, index) => {
        return (item = 2 * item);
    });
    map.set("test replaceAllElements:", subVec.toString() == "8,6,16");
    // GetFirstElement
    map.set("test GetFirstElement:", subVec.getFirstElement() == 8 &&
            vec.getFirstElement() == 4);

    let arr = [4, 3, 8, 1, 5, 14];
    for (let i = 0; i < vector.length; i++) {
        map.set("for of " + arr[i], vec.get(i) == arr[i]);
    }

    let flag = false;
    try {
        vec["aa"] = 3;
    } catch (e) {
        flag = true;
    }

    map.set("test vector throw error", flag);

    let vector1 = new FastVector();
    let proxy = new Proxy(vector1, {});
    let testArray = []
    res = true
    for(let i = 0; i<10; i++) {
        proxy.add(i)
        testArray.push(i)
    }

    for(let i = 0; i < testArray.length; i++) {
        if (proxy[i] !== testArray[i]) {
            res = false
        }
    }
    map.set("test vector add:", res)

    map.set("test vector has:", proxy.has(2))
    map.set("test vector getCapacity:", proxy.getCapacity() === 10)
    map.set("test vector getLastIndexOf:", proxy.getLastIndexOf(1) === 1)
    map.set("test vector getIndexOf:", proxy.getIndexOf(5) === 5)
    map.set("test vector getFirstElement:", proxy.getFirstElement() === 0)
    map.set("test vector getLastElement:", proxy.getLastElement() === 9)
    map.set("test vector getLastIndexFrom:", proxy.getLastIndexFrom(1, 5) === 1)
    map.set("test vector getIndexFrom:", proxy.getIndexFrom(5, 1) === 5)
    
    let array = proxy.convertToArray()
    res = true
    for(let i = 0; i < testArray.length; i++) {
        if (array[i] !== testArray[i]) {
            res = false
        }
    }
    map.set("test vector convertToArray:", res)

    let newVector = proxy.clone()
    res = true
    for(let i = 0; i < testArray.length; i++) {
        if (newVector[i] !== testArray[i]) {
            res = false
        }
    }
    map.set("test vector clone:", res)

    proxy.insert(999, 3)
    testArray.splice(3, 0, 999)
    res = true
    for(let i = 0; i < testArray.length; i++) {
        if (proxy[i] !== testArray[i]) {
            res = false
        }
    }
    map.set("test vector insert:", res)

    proxy.removeByIndex(9)
    testArray.splice(9, 1)
    res = true
    for(let i = 0; i < testArray.length; i++) {
        if (proxy[i] !== testArray[i]) {
            res = false
        }
    }
    map.set("test vector removeByIndex:", res)

    const removeRes = proxy.remove(6)
    testArray.splice(7, 1)
    res = true
    for(let i = 0; i < testArray.length; i++) {
        if (proxy[i] !== testArray[i]) {
            res = false
        }
    }
    map.set("test vector remove:", res)
    map.set("test vector removeRes:", removeRes)

    proxy.removeByRange(1, 3)
    testArray.splice(1, 2)
    res = true
    for(let i = 0; i < testArray.length; i++) {
        if (proxy[i] !== testArray[i]) {
            res = false
        }
    }
    map.set("test vector removeByRange:", res)

    res = true
    proxy.forEach((value, index) => {
        if (value !== testArray[index]) {
            res = false
        }
    })

    map.set("test vector forEach:", res)

    proxy.replaceAllElements((item, index) => {
        return item * 2
    })
    res = true
    for(let i = 0; i < testArray.length; i++) {
        if (proxy[i] !== testArray[i] * 2) {
            res = false
        }
    }
    map.set("test vector replaceAllElements:", res)

    res = true
    let subVector = proxy.subVector(1, 3)
    const newtestArray = testArray.slice(1, 3)
    for(let i = 0; i < subVector.length; i++) {
        if (newtestArray[i] * 2 !== subVector[i]) {
            res =  false
        }
    }
    map.set("test vector subVector:", res)

    res = true
    let j = 0
    for (const data of proxy) {
      if (data !== testArray[j] * 2) {
        res = false
      }
      j++;
    }
    map.set("test vector for of:", res)

    let itr = proxy[Symbol.iterator]();
    let tmp = undefined;
    let testArray1 = []
    do {
      tmp = itr.next().value;
      testArray1.push(tmp);
    } while (tmp != undefined);

    for (let k = 0; k < proxy.length; k++) {
      if (testArray1[k] !== testArray[k] * 2) {
        res = false
      }
    }
    map.set("test vector Symbol.iterator:", res)

    let vector2 = new FastVector();
    let proxy1 = new Proxy(vector2, {});
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
    map.set("test vector sort:", res)

    map.set("test vector get:", proxy1.get(2) === 2)
    proxy1.set(3, 7)
    map.set("test vector set and get:", proxy1.get(3) === 7)

    proxy1.clear()
    map.set("test vector clear:", proxy1.length === 0)
    map.set("test vector isEmpty:", proxy1.isEmpty())
    proxy1.add(4);
    proxy1.add(3);
    proxy1.add(1);
    proxy1.add(2);
    proxy1.add(0);
    map.set("test vector toString:", proxy1.toString() == "4,3,1,2,0");

    res = true
    let arr1 = [-1, -1, -1, -1, -1];
    proxy1.copyToArray(arr1);
    let a = [4, 3, 1, 2, 0];
    for (let i = 0; i < a.length; i++) {
        if (arr1[i] !== a[i]) {
            res = false
        }
    }
    map.set("test vector copyToArray:", res);

    proxy1.sort((a,b) => a-b);
    res = true
    for (let i = 0; i < 5; i++) {
        if (proxy1[i] !== i) {
            res = false
        }
    }
    map.set("test vector sort:", res)

    proxy1.increaseCapacityTo(20)
    map.set("test vector increaseCapacityTo:", proxy1.getCapacity() === 20)

    proxy1.trimToCurrentLength()
    map.set("test vector trimToCurrentLength:", proxy1.getCapacity() === 5)

    let v = new FastVector();
    for (let i = 0; i < 10; i++) {
        v.add(i);
    }
    v.replaceAllElements((value, key, t) => {
        if (key == 8) {
            t.clear();
            t.trimToCurrentLength();
            return 0;
        }
        return value;
    }, v);
    map.set("test vector replaceAllElements redefine:", v.length == 0);

    let isClear = false;
    let tempArr = new FastVector();
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
    map.set("test vector sort callbackFN:", tempArr.length === 3);
    
    flag = undefined;
    function elements(value, key, map) {
        if (!value) {
            if (!flag) {
                flag = [];
            }
            flag.push(key);
        }
    }

    let de = new FastVector();
    try {
        de.forEach(123);
    } catch(err) {
        if (err.name != "TypeError") {
            print("Vector forEach throw error fail");
        }
    }
    map.forEach(elements);
    let test1 = new FastVector();
    for (let k = 0; k < 10; k++) {
        test1.add(k);
    }
    var keyName = "";
    for (const key in test1) {
        keyName += key;
    }
    if (keyName != "0123456789") {
        print("Vector for in fail")
    }
    if (!flag) {
        print("Test Vector success!!!");
    } else {
        print("Test Vector fail: " + flag);
    }
}
export let vectorRes = "Test Vector done";