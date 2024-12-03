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
import {testCommon, testdProxySet, testdProxyIterator}  from "./utility";
export let treemapRes = "Test TreeMap done";

var fastmap = undefined;
if (globalThis["ArkPrivate"] != undefined) {
    fastmap = ArkPrivate.Load(ArkPrivate.TreeMap);

    let res = new Map();
    let map = new fastmap();
    map.set("a", "aa");
    map.set("b", "bb");

    // test get: true
    res.set("test get:", map.length == 2 && map.get("a") == "aa" && map.get("b") == "bb");
    // test hasKey and hasValue: true
    res.set("test hasKey and hasValue:", map.hasKey("a") && map.hasKey("b") && map.hasValue("aa") &&
            map.hasValue("bb") && !map.hasKey("c") && !map.hasValue("cc"));

    map.set("c", "cc");
    // test getFirstKey and getLastKey: true
    res.set("test getFirstKey and getLastKey:", map.getFirstKey() == "a" && map.getLastKey() == "c");
    // test getLowerKey and getHigherKey: true
    res.set("test getLowerKey and getHigherKey:", map.getLowerKey("b") == "a" && map.getLowerKey("a") == undefined &&
            map.getHigherKey("b") == "c" && map.getHigherKey("c") == undefined);
    
    testdProxyIterator(map, res);

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

    let flag = false;
    try {
        map["aa"] = 3;
    } catch (e) {
        flag = true;
    }
    res.set("test map throw error", flag);

    let map1 = new fastmap();
    let proxy = new Proxy(map1, {});

    // test proxy isEmpty: true
    res.set("test proxy isEmpty true:", proxy.isEmpty() == true)

    proxy.set("a", "aa");
    proxy.set("b", "bb");

    // test proxy isEmpty: false
    res.set("test proxy isEmpty false:", proxy.isEmpty() == false)

    // test get: true
    res.set("test get:", proxy.length == 2 && proxy.get("a") == "aa" && proxy.get("b") == "bb");
    // test hasKey and hasValue: true
    res.set("test hasKey and hasValue:", proxy.hasKey("a") && proxy.hasKey("b") && proxy.hasValue("aa") &&
            proxy.hasValue("bb") && !proxy.hasKey("c") && !proxy.hasValue("cc"));

    proxy.set("c", "cc");
    // test getFirstKey and getLastKey: true
    res.set("test getFirstKey and getLastKey:", proxy.getFirstKey() == "a" && proxy.getLastKey() == "c");
    // test getLowerKey and getHigherKey: true
    res.set("test getLowerKey and getHigherKey:", proxy.getLowerKey("b") == "a" && proxy.getLowerKey("a") == undefined &&
            proxy.getHigherKey("b") == "c" && proxy.getHigherKey("c") == undefined);

    testCommon(proxy, res);
    // test forEach:
    flag = false;
    function TestForEachTreeMap(valueTreeMap, keyTreeMap, proxy) {
        flag = proxy.get(keyTreeMap) === valueTreeMap;
        res.set("test forEach" + keyTreeMap, flag)
    }
    proxy.forEach(TestForEachTreeMap);

    let dmap1 = new fastmap();
    let dProxy = new Proxy(dmap1, {});
    testdProxySet(proxy, res, dProxy);

    let cmmp = new fastmap((firstValue, secondValue) => {return firstValue > secondValue});
    cmmp.set("aa", 1);
    cmmp.set("bb", 1);
    res.set("test hasKey undefined", cmmp.hasKey(undefined) == false);
    res.set("test hasKey null", cmmp.hasKey(null) == false);
    cmmp.set(null, 1);
    cmmp.set(undefined, 1);
    res.set("test hasKey undefined true", cmmp.hasKey(undefined) == true);
    res.set("test hasKey null true", cmmp.hasKey(null) == true);

    cmmp.clear();
    let commap1 = new fastmap();
    commap1.setAll(cmmp);
    res.set("test setAll null map", commap1.length == 0);

    let commap = new fastmap((firstValue, secondValue) => {return firstValue > secondValue});
    commap.set("c","1");
    commap.set("a","8");
    commap.set("b","2");
    commap.set("d","4");
    if (commap.length == 4) {
        commap.remove("a");
        commap.remove("b");
        commap.remove("c");
        commap.remove("d");
    }
    res.set("test commpare", commap.length == 0);
    class Person {
        id = 0;
        name = '';
        constructor(id, name) {
            this.id = id;
            this.name = name;
        }
    }
    commap = new fastmap((firstValue, secondValue) => {return firstValue.id > secondValue.id});
    let personone = new Person(1,'张三');
    let persontwo = new Person(3,'李四');
    let personsec = new Person(2,'王五');
    commap.set(personone,"1")
    commap.set(persontwo,"1")
    commap.set(personsec,"1")
    res.set("test clear and set", commap.getFirstKey().id === 3);
    commap.clear();
    commap = new fastmap((firstValue, secondValue) => {return firstValue < secondValue});
    commap.set("c","1");
    commap.set("a","8");
    commap.set("b","2");
    commap.set("d","4");
    commap.clear();
    commap.set("c","1");
    commap.set("a","8");
    commap.set("b","2");
    commap.set("d","4");
    res.set("test clear and set", commap.getFirstKey() === "a");

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

    let de = new fastmap();
    try {
        de.forEach(123);
    } catch(err) {
        if (err.name != "BusinessError") {
            print("TreeMap forEach throw error fail");
        }
    }
    if (!flag) {
        print("Test TreeMap success!!!");
    } else {
        print("Test TreeMap fail: " + flag);
    }
    map = new fastmap();
    map.set("a", "av");
    map.clear();
    map.set("b", "bv");
    map.set("c", "cv");
    print("Test TreeMap set After Clear Success")
    
    let growMap = new fastmap();
    const keyStr = 'KEY_TEST_MAP';
    growMap.set(keyStr,21)
    growMap.set(keyStr,68)
    growMap.remove(keyStr)
    growMap.set(keyStr,23)
    growMap.remove(keyStr)
    growMap.set(keyStr,86)
    growMap.remove(keyStr)
    growMap.set(keyStr,37)
    growMap.remove(keyStr)
    growMap.set(keyStr,33)
    growMap.remove(keyStr)
    growMap.set(keyStr,34)
    growMap.remove(keyStr)
    growMap.set(keyStr,64)
    growMap.set(keyStr,100)
    print(growMap.get(keyStr))
}
