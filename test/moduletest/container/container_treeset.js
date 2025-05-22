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
var fastset = undefined;
class c{
    n = 0;
    constructor(a){
      this.n = a;
    }
  }
if (globalThis["ArkPrivate"] != undefined) {
    fastset = ArkPrivate.Load(ArkPrivate.TreeSet);

    let map = new Map();
    let set = new fastset();
    set.add("aa");
    set.add("bb");

    // test has: true
    map.set("test has:", set.length == 2 && set.has("aa") && set.has("bb") && !set.has("cc"));

    set.add("cc");
    // test getFirstKey and getLastKey: true
    map.set("test getFirstKey and getLastKey:", set.getFirstValue() == "aa" && set.getLastValue() == "cc");
    // test getLowerValue and getHigherValue out: true
    map.set("test getLowerValue and getHigherValue", set.getLowerValue("bb") == "aa" &&
            set.getLowerValue("aa") == undefined && set.getHigherValue("bb") == "cc" &&
            set.getHigherValue("cc") == undefined);

    // test values: true
    let iteratorSetValues = set.values();
    map.set("test values:", iteratorSetValues.next().value == "aa" && iteratorSetValues.next().value == "bb" &&
            iteratorSetValues.next().value == "cc" && iteratorSetValues.next().value == undefined);
    // test entries: [cc, cc], undefined
    let iteratorSetEntries = set.entries();
    iteratorSetEntries.next().value;
    iteratorSetEntries.next().value;
    map.set("test entries1:", iteratorSetEntries.next().value != undefined);
    map.set("test entries2:", iteratorSetEntries.next().value == undefined);

    // test forof: aa, bb, cc
    let arr = ["aa", "bb", "cc"];
    let i = 0;
    for (const item of set) {
        map.set(arr[i], item == arr[i]);
        i++;
    }

    // test forin:
    for (const item in set) {
        map.set("test forin:", item);
    }

    // test forEach:
    let setFlag = false;
    function TestForEach(value, key, set) {
        setFlag= set.has(key) && set.has(value);
        map.set("test forEach" + key, setFlag);
    }
    set.forEach(TestForEach);

    // test isEmpty: false
    map.set("test isEmpty:", !set.isEmpty());

    set.add("ee");
    set.add("dd");
    // test popFirst and popLast: true
    map.set("test popFirst and popLast:", set.length == 5 && set.popFirst() == "aa" &&
          set.popLast() == "ee" && !set.has("aa"));
    // test remove: true
    map.set("test remove:", set.remove("bb") && set.length == 2 && !set.has("bb"));
    // test clear: true
    set.clear();
    map.set("test clear:", set.length == 0 && !set.has("cc") && set.isEmpty());

    let flag = false;
    try {
        set["aa"] = 3;
    } catch (e) {
        flag = true;
    }
    map.set("test set throw error", flag);

    let cmmp = new fastset((firstValue, secondValue) => {return firstValue > secondValue});
    cmmp.add("aa");
    cmmp.add("bb");
    map.set("test has undefined", cmmp.has(undefined) == false);
    map.set("test has null", cmmp.has(null) == false);
    cmmp.add(null, 1);
    cmmp.add(undefined, 1);
    map.set("test has undefined false", cmmp.has(undefined) == true);
    map.set("test has null false", cmmp.has(null) == true);

    let comset =  new fastset((firstValue, secondValue) => {return firstValue < secondValue});
    comset.add("c");
    comset.add("a");
    comset.add("b");
    comset.add("d");
    if (comset.length == 4) {
        comset.remove("a");
        comset.remove("b");
        comset.remove("c");
        comset.remove("d");
    }
    map.set("test commpare", comset.length == 0);

    class Person {
        id = 0;
        name = '';
        constructor(id, name) {
            this.id = id;
            this.name = name;
        }
    }

    comset =  new fastset((firstValue, secondValue) => {return firstValue.id < secondValue.id});
    let personone = new Person(3,'张三');
    let persontwo = new Person(1,'李四');
    let personsec = new Person(2,'王五');
    comset.add(personone);
    comset.add(persontwo);
    comset.add(personsec);
    map.set("test clear and set", comset.getFirstValue().id === 1);
    comset.clear();
    comset =  new fastset((firstValue, secondValue) => {return firstValue > secondValue});
    comset.add("c");
    comset.add("a");
    comset.add("b");
    comset.add("d");
    comset.clear();
    comset.add("c");
    comset.add("a");
    comset.add("b");
    comset.add("d");
    map.set("test clear and set", comset.getFirstValue() === "d");

    let set1 = new fastset();
    let proxy = new Proxy(set1, {});
    proxy.add("aa");
    proxy.add("bb");

    // test has: true
    map.set("test has:", proxy.length == 2 && proxy.has("aa") && proxy.has("bb") && !proxy.has("cc"));

    proxy.add("cc");
    // test getFirstKey and getLastKey: true
    map.set("test getFirstKey and getLastKey:", proxy.getFirstValue() == "aa" && proxy.getLastValue() == "cc");
    // test getLowerValue and getHigherValue out: true
    map.set("test getLowerValue and getHigherValue", proxy.getLowerValue("bb") == "aa" &&
            proxy.getLowerValue("aa") == undefined && proxy.getHigherValue("bb") == "cc" &&
            proxy.getHigherValue("cc") == undefined);

    // test values: true
    let iteratorSetValues1 = proxy.values();
    map.set("test values:", iteratorSetValues1.next().value == "aa" && iteratorSetValues1.next().value == "bb" &&
            iteratorSetValues1.next().value == "cc" && iteratorSetValues1.next().value == undefined);
    // test entries: [cc, cc], undefined
    let iteratorSetEntries1 = proxy.entries();
    iteratorSetEntries1.next().value;
    iteratorSetEntries1.next().value;
    map.set("test entries1:", iteratorSetEntries1.next().value != undefined);
    map.set("test entries2:", iteratorSetEntries1.next().value == undefined);

    // test forof: aa, bb, cc
    let arr1 = ["aa", "bb", "cc"];
    let j = 0;
    for (const item of proxy) {
        map.set(arr1[j], item == arr1[j]);
        j++;
    }

    // test forin:
    for (const item in proxy) {
        map.set("test forin:", item);
    }

    // test forEach:
    let setFlag1 = false;
    function TestForEach1(value, key, proxy) {
        setFlag1 = proxy.has(key) && proxy.has(value);
        map.set("test forEach" + key, setFlag1);
    }
    proxy.forEach(TestForEach1);

    // test isEmpty: false
    map.set("test isEmpty:", !proxy.isEmpty());

    proxy.add("ee");
    proxy.add("dd");
    // test popFirst and popLast: true
    map.set("test popFirst and popLast:", proxy.length == 5 && proxy.popFirst() == "aa" &&
            proxy.popLast() == "ee" && !proxy.has("aa"));
    // test remove: true
    map.set("test remove:", proxy.remove("bb") && proxy.length == 2 && !proxy.has("bb"));
    // test clear: true
    proxy.clear();
    map.set("test clear:", proxy.length == 0 && !proxy.has("cc") && proxy.isEmpty());

    flag = false;
    try {
        proxy["aa"] = 3;
    } catch (e) {
        flag = true;
    }
    map.set("test set throw error", flag);

    // test getLower & getHigher when object
    let newset = new fastset((x, y)=> x.n < y.n);
    newset.add(new c(3));
    let tmp_c = new c(5);
    newset.add(tmp_c);
    newset.add(undefined);
    newset.add(new c(1));
    newset.add(null);
    map.set("test getHigher no.1:", newset.getHigherValue(new c(3)).n == 5);
    map.set("test getHigher no.2:", newset.getHigherValue(new c(5)) == null);
    map.set("test getHigher no.3:", newset.getHigherValue(null) == undefined);
    map.set("test getLower no.1:", newset.getLowerValue(new c(3)).n == 1);
    map.set("test getLower no.2:", newset.getLowerValue(undefined) == null);
    map.set("test getLower no.3:", newset.getLowerValue(null) == tmp_c);
{
    class A {
        time = 0;
        constructor(time) {
            this.time = time;
        }
        compared = ((first, second) => {
            return first.time - second.time;
        });
    }
    let set = new fastset(A.compared);
    const a1 = new A(1);
    const a2 = new A(2);
    const a3 = new A(3);
    const a4 = new A(4);
    const a5 = new A(5);
    set.add(a1);
    set.add(a2);
    set.add(a3);
    set.add(a4);
    set.add(a5);
    for (let i = 0; i < 5; i++) {
        set.remove(a1);
        let ok = set.has(a1);
        map.set("test add and remove failed, expect:" + false + ", output:" + ok, ok === false);
        set.add(a1);
    }
}

    flag = undefined;
    function elementsTreeSet(valueTreeSet, keyTreeSet, map) {
        if (!valueTreeSet) {
            if (!flag) {
                flag = [];
            }
            flag.push(keyTreeSet);
        }
    }
    map.forEach(elementsTreeSet);

    let de = new fastset();
    try {
        de.forEach(123);
    } catch(err) {
        if (err.name != "BusinessError") {
            print("TreeSet forEach throw error fail");
        }
    }
    if (!flag) {
        print("Test TreeSet success!!!");
    } else {
        print("Test TreeSet fail: " + flag);
    }
    let treeSet = new fastset((first,second) =>{
        return first > second
    });
    let insertArr = [
        643,
        811,
        807,
        378,
        226,
        195,
        599,
        641,
        494,
        964,
        156,
        419,
        977,
        20,
        788,
        596
    ]
    let addItem = function(obj){
        treeSet.add(obj)
    }
    let removeItem = function(){
        const first = treeSet.getFirstValue()
        treeSet.remove(first)
    }
    for(let i = 0;i < insertArr.length;i++) {
        addItem(insertArr[i])
    }
    removeItem()
    removeItem()
    removeItem()
    removeItem()
    removeItem()
    addItem(664)
    removeItem()
    removeItem()
    removeItem()
    removeItem()
    removeItem()
    removeItem()
    removeItem()
    removeItem()
    let resArr = []
    treeSet.forEach(element => {
        resArr.push(element)
    });
    print(resArr)
}
export let treesetRes = "Test TreeSet done";