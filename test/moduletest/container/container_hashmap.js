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
var fastmap = undefined;
if (globalThis["ArkPrivate"] != undefined) {
    fastmap = ArkPrivate.Load(ArkPrivate.HashMap);

    let res = new Map();
    let map = new fastmap();

    // test isEmpty true
    res.set("test isEmpty ture:", map.isEmpty() == true)

    map.set("a", "aa");
    map.set("b", "bb");

    // test isEmpty false
    res.set("test isEmpty false:", map.isEmpty() == false)
    // test get: true
    res.set("test get:", map.length == 2 && map.get("a") == "aa" && map.get("b") == "bb");
    // test hasKey and hasValue: true
    res.set("test hasKey and hasValue:", map.hasKey("a") && map.hasKey("b") && map.hasValue("aa") &&
            map.hasValue("bb") && !map.hasKey("c") && !map.hasValue("cc"));

    map.set("c", "cc");
    // test keys: true
    let iteratorKey = map.keys();
    res.set("test keys:", iteratorKey.next().value == "a" && iteratorKey.next().value == "b" &&
            iteratorKey.next().value == "c" && iteratorKey.next().value == undefined);
    // test values: true
    let iteratorValues = map.values();
    res.set("test values:", iteratorValues.next().value == "aa" && iteratorValues.next().value == "bb" &&
            iteratorValues.next().value == "cc" && iteratorValues.next().value == undefined);
    // test entries: [c,cc], undefined
    let iteratorEntries = map.entries();
    iteratorEntries.next().value;
    iteratorEntries.next().value;
    res.set("test entries1:", iteratorEntries.next().value != undefined);
    res.set("itest entries2:", iteratorEntries.next().value == undefined);

    // test forof: [a, aa], [b, bb], [c, cc]
    let arr = ["aa", "bb", "cc"];
    let i = 0;
    for (const item of map) {
        res.set(arr[i], item[1] == arr[i]);
        i++;
    }
    // test forin:
    for (const item in map) {
        res.set("test forin", false);
    }
    // test forEach:
    let flag = false;
    function TestForEach(value, key, map) {
        flag = map.get(key) === value;
        res.set("test forEach" + key, flag)
    }
    map.forEach(TestForEach);

    let dmap = new fastmap();
    // test setAll: 3
    dmap.setAll(map);
    res.set("test setAll:", dmap.length == 3);
    // test remove: true
    res.set("test remove:", dmap.remove("a") == "aa" && dmap.length == 2);
    // test replace: true
    res.set("test replace:", dmap.replace("b", "dd") && dmap.get("b") == "dd");
    // test clear: 0
    dmap.clear();
    res.set("test clear:", dmap.length == 0);

    flag = false;
    try {
        map["aa"] = 3;
    } catch (e) {
        flag = true;
    }
    res.set("test map throw error", flag);

    let map1 = new fastmap();
    let proxy = new Proxy(map1, {});

    // test isEmpty true
    res.set("test proxy isEmpty ture:", proxy.isEmpty() == true)

    proxy.set("a", "aa");
    proxy.set("b", "bb");

    // test isEmpty false
    res.set("test proxy isEmpty false:", proxy.isEmpty() == false)

    // test get: true
    res.set("test get:", proxy.length == 2 && proxy.get("a") == "aa" && proxy.get("b") == "bb");
    // test hasKey and hasValue: true
    res.set("test hasKey and hasValue:", proxy.hasKey("a") && proxy.hasKey("b") && proxy.hasValue("aa") &&
            proxy.hasValue("bb") && !proxy.hasKey("c") && !proxy.hasValue("cc"));

    proxy.set("c", "cc");
    // test keys: true
    let iteratorKey1 = proxy.keys();
    res.set("test keys:", iteratorKey1.next().value == "a" && iteratorKey1.next().value == "b" &&
            iteratorKey1.next().value == "c" && iteratorKey1.next().value == undefined);
    // test values: true
    let iteratorValues1 = proxy.values();
    res.set("test values:", iteratorValues1.next().value == "aa" && iteratorValues1.next().value == "bb" &&
            iteratorValues1.next().value == "cc" && iteratorValues1.next().value == undefined);
    // test entries: [c,cc], undefined
    let iteratorEntries1 = proxy.entries();
    iteratorEntries1.next().value;
    iteratorEntries1.next().value;
    res.set("test entries1:", iteratorEntries1.next().value != undefined);
    res.set("itest entries2:", iteratorEntries1.next().value == undefined);

    // test forof: [a, aa], [b, bb], [c, cc]
    let arr1 = ["aa", "bb", "cc"];
    let j = 0;
    for (const item of proxy) {
        res.set(arr1[j], item[1] == arr1[j]);
        j++;
    }
    // test forin:
    for (const item in proxy) {
        res.set("test forin", false);
    }
    // test forEach:
    flag = false;
    function TestForEach1(value, key, proxy) {
        flag = proxy.get(key) === value;
        res.set("test forEach" + key, flag)
    }
    proxy.forEach(TestForEach1);

    let dmap1 = new fastmap();
    let dProxy = new Proxy(dmap1, {})
    // test setAll: 3
    dProxy.setAll(proxy);
    res.set("test setAll:", dProxy.length == 3);
    // test remove: true
    res.set("test remove:", dProxy.remove("a") == "aa" && dProxy.length == 2);
    // test replace: true
    res.set("test replace:", dProxy.replace("b", "dd") && dProxy.get("b") == "dd");
    // test clear: 0
    dProxy.clear();
    res.set("test clear:", dProxy.length == 0);

    flag = false;
    try {
        proxy["aa"] = 3;
    } catch (e) {
        flag = true;
    }
    res.set("test map throw error", flag);
    flag = undefined;
    function elements(value, key, map) {
        if (!value) {
            if (!flag) {
                flag = [];
            }
            flag.push(key);
        }
    }
    res.forEach(elements);

    // test RBTree
    let collisionMap = new fastmap();
    let count = 0;
    // same hash when mod 1024
    collisionMap.set(1224, 1);
    collisionMap.set(1285, 2);
    collisionMap.set(1463, 3);
    collisionMap.set(4307, 4);
    collisionMap.set(5135, 5);
    collisionMap.set(5903, 6);
    collisionMap.set(6603, 7);
    collisionMap.set(6780, 8);
    collisionMap.set(8416, 9);
    collisionMap.set(9401, 10);
    collisionMap.set(9740, 11);
    collisionMap.forEach((value, key, hashMap) => {
        if (hashMap.get(key) == value) {
            count += value;
        }
    });
    if (count != 66) {  // 66: 1 + 2 + 3 + ... + 11
        print("test RBTree forEach fail. count=" + count);
    }

    let de = new fastmap();
    try {
        de.forEach(123);
    } catch(err) {
        if (err.name != "BusinessError") {
            print("HashMap forEach throw error fail");
        }
    }
    if (!flag) {
        print("Test HashMap success!!!");
    } else {
        print("Test HashMap fail: " + flag);
    }
}
