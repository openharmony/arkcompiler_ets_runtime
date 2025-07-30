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

    class Tmp {
        age = 0;
        constructor(age) {
            this.age = age;
        }
    }

    {
        let plainArray = new PlainArray();
        plainArray.add(1, "squirrel");
        plainArray.add(2, "sparrow");
        let result = plainArray.get(1);
        map.set("test PlainArray get0001:", result == "squirrel");
    }

    {
        let plainArray = new PlainArray();
        plainArray.add(1, "");
        plainArray.add(2, "sparrow");
        let result = plainArray.get(1);
        map.set("test PlainArray get0002:", result == '');
    }

    {
        try {
            let plainArray = new PlainArray();
            plainArray.add(null, "zhang san");
            plainArray.get(null);
        } catch (err) {
            const code = err.code;
            let result = (code == 401);
            map.set("test PlainArray get0003:", result == true);
        }
    }

    {
        try {
            let plainArray = new PlainArray();
            plainArray.add(undefined, "zhang san");
            plainArray.get(undefined);
        } catch (err) {
            const code = err.code;
            let result = (code == 401);
            map.set("test PlainArray get0004:", result == true);
        }
    }

    {
        let plainArray = new PlainArray();
        plainArray.add(0, "zhang san");
        let str = plainArray.get(0);
        let result = (str == "zhang san");
        map.set("test PlainArray get0005:", result == true);
        plainArray.add(-1, "zhang san");
        str = plainArray.get(-1);
        result = (str == "zhang san");
        map.set("test PlainArray get0005:", result == true);

        let num = 2147483647;
        plainArray.add(num, "li si");
        str = plainArray.get(num);
        result = (str == "li si");
        map.set("test PlainArray get0005:", result == true);
        let num1 = -2147483648;
        plainArray.add(num1, "li si");
        str = plainArray.get(num1);
        result = (str == "li si");
        map.set("test PlainArray get0005:", result == true);
    }
    
    {
        let plainArray = new PlainArray();
        plainArray.add(1, "squirrel");
        plainArray.add(2, "");
        let result = plainArray.getIndexOfKey(2);
        map.set("test PlainArray getIndexOfKey0001:", result == 1);
    }

    {
        let plainArray = new PlainArray();
        plainArray.add(1, new Tmp(10));
        plainArray.add(2, new Tmp(12));
        let result = plainArray.getIndexOfKey(2);
        map.set("test PlainArray getIndexOfKey0002:", result == 1);
    }

    {
        let plainArray = new PlainArray();
        plainArray.add(1, "squirrel");
        plainArray.add(2, null);
        plainArray.add(3, undefined);
        let result = plainArray.getIndexOfKey(2);
        map.set("test PlainArray getIndexOfKey0003:", result == 1);
        let result1 = plainArray.getIndexOfKey(3);
        map.set("test PlainArray getIndexOfKey0003:", result1 == 2);
    }

    {
        let plainArray = new PlainArray();
        plainArray.add(1, "squirrel");
        plainArray.add(2, "sparrow");
        let result = plainArray.getIndexOfKey(2);
        map.set("test PlainArray getIndexOfKey0004:", result == 1);
    }

    {
        let plainArray = new PlainArray();
        plainArray.add(1, "squirrel");
        plainArray.add(2, "");
        let result = plainArray.getIndexOfValue("");
        map.set("test PlainArray getIndexOfValue0001:", result == 1);
    }

    {
        let plainArray = new PlainArray();
        let param1 = new Tmp(10);
        plainArray.add(1, param1);
        let param2 = new Tmp(10);
        plainArray.add(2, param2);
        let result = plainArray.getIndexOfValue(param2);
        map.set("test PlainArray getIndexOfValue0002:", result == 1);
    }

    {
        let plainArray = new PlainArray();
        plainArray.add(1, "squirrel");
        plainArray.add(2, null);
        plainArray.add(3, undefined);
        let result = plainArray.getIndexOfValue(null);
        map.set("test PlainArray getIndexOfValue0003:", result == 1);
        let result1 = plainArray.getIndexOfValue(undefined);
        map.set("test PlainArray getIndexOfValue0003:", result1 == 2);
    }

    {
        let plainArray = new PlainArray();
        plainArray.add(1, "squirrel");
        plainArray.add(2, "sparrow");
        let result = plainArray.getIndexOfValue("sparrow");
        map.set("test PlainArray getIndexOfValue0004:", result == 1);
    }

    {
        let plainArray = new PlainArray();
        plainArray.add(1, "squirrel");
        plainArray.add(2, "sparrow");
        let result = plainArray.getKeyAt(3);
        map.set("test PlainArray getKeyAt0001:", result == undefined);
    }

    {
        let plainArray = new PlainArray();
        plainArray.add(1, "squirrel");
        plainArray.add(2, "sparrow");
        let result = plainArray.getKeyAt(1);
        map.set("test PlainArray getKeyAt0002:", result == 2);
    }

    {
        try {
            let plainArray = new PlainArray();
            plainArray.add(1, "squirrel");
            plainArray.add(2, "sparrow");
            plainArray.getKeyAt(null);
        } catch (err) {
            const code = err.code;
            let result = (code == 401);
            map.set("test PlainArray getKeyAt0003:", result == true);
        }
    }

    {
        try {
            let plainArray = new PlainArray();
            plainArray.add(1, "squirrel");
            plainArray.add(2, "sparrow");
            plainArray.getKeyAt(undefined);
        } catch (err) {
            const code = err.code;
            let result = (code == 401);
            map.set("test PlainArray getKeyAt0004:", result == true);
        }
    }

    {
        let plainArray = new PlainArray();
        plainArray.add(1, "squirrel");
        plainArray.add(2, "sparrow");
        let result = plainArray.getKeyAt(1);
        let index = plainArray.getIndexOfKey(result);
        map.set("test PlainArray getKeyAt0005:", index == 1);
    }

    {
        try {
            let plainArray = new PlainArray();
            plainArray.add(1, "squirrel");
            plainArray.add(2, "sparrow");
            plainArray.getValueAt(2);
        } catch (err) {
            const code = err.code;
            let result = (code == 10200001);
            map.set("test PlainArray getValueAt0001:", result == true);
        }
    }

    {
        let plainArray = new PlainArray();
        plainArray.add(1, "squirrel");
        plainArray.add(2, "sparrow");
        let result = plainArray.getValueAt(1);
        map.set("test PlainArray getValueAt0002:", result == "sparrow");
    }

    {
        try {
            let plainArray = new PlainArray();
            plainArray.add(1, "squirrel");
            plainArray.add(2, "sparrow");
            plainArray.getValueAt(null);
        } catch (err) {
            const code = err.code;
            let result = (code == 401);
            map.set("test PlainArray getValueAt0003:", result == true);
        }
    }

    {
        try {
            let plainArray = new PlainArray();
            plainArray.add(1, "squirrel");
            plainArray.add(2, "sparrow");
            plainArray.getValueAt(undefined);
        } catch (err) {
            const code = err.code;
            let result = (code == 401);
            map.set("test PlainArray getValueAt0004:", result == true);
        }
    }

    {
        let plainArray = new PlainArray();
        plainArray.add(1, "squirrel");
        plainArray.add(2, "sparrow");
        let result = plainArray.getValueAt(1);
        let index = plainArray.getIndexOfValue(result);
        map.set("test PlainArray getValueAt0005:", index == 1);
    }

    {
        let plainArray = new PlainArray();
        plainArray.add(1, "squirrel");
        plainArray.add(2, "sparrow");
        let newPlainArray = plainArray.clone();
        let result = (newPlainArray.length == plainArray.length);
        map.set("test PlainArray clone0001", result == true);
    }
    
    {
        let plainArray = new PlainArray();
        plainArray.add(1, 1);
        plainArray.add(2, 2);
        let newPlainArray = plainArray.clone();
        let result = (newPlainArray.length == plainArray.length);
        map.set("test PlainArray clone0002", result == true);
    }

    {
        let plainArray = new PlainArray();
        plainArray.add(1, new Tmp(10));
        plainArray.add(2, new Tmp(12));
        let newPlainArray = plainArray.clone();
        let result = (newPlainArray.length == plainArray.length);
        map.set("test PlainArray clone0003", result == true);
    }

    {
        let plainArray = new PlainArray();
        plainArray.add(1, new Tmp(10));
        plainArray.add(2, new Tmp(12));
        let newPlainArray = plainArray.clone();
        newPlainArray.add(3, new Tmp(13));
        let result = plainArray.length;
        map.set("test PlainArray clone0004", result == 2);
    }

    {
        let plainArray = new PlainArray();
        let param = "a".repeat(5120);
        plainArray.add(1, param);
        let newPlainArray = plainArray.clone();
        let result = (newPlainArray.length == plainArray.length);
        map.set("test PlainArray clone0005", result == true);
    }

    {
        let plainArray = new PlainArray();
        plainArray.add(1, null);
        plainArray.add(2, undefined);
        let newPlainArray = plainArray.clone();
        let result = (newPlainArray.length == plainArray.length);
        map.set("test PlainArray clone0006", result == true);
    }

    {
        let plainArray = new PlainArray();
        let param = "a".repeat(5120);
        plainArray.add(1, param);
        let result = plainArray.length;
        map.set("test PlainArray add0001", result == 1);
    }

    {
        let plainArray = new PlainArray();
        plainArray.add(1, "squirrel");
        let result = plainArray.length;
        map.set("test PlainArray add0002", result == 1);
    }

    {
        let plainArray = new PlainArray();
        plainArray.add(1, null);
        plainArray.add(2, undefined);
        let result = plainArray.length;
        map.set("test PlainArray add0003", result == 2);
    }

    {
        let plainArray = new PlainArray();
        plainArray.add(1, "squirrel");
        plainArray.add(2, "sparrow");
        let result = plainArray.remove(2);
        map.set("test PlainArray remove0001", result == "sparrow");
    }

    {
        try {
            let plainArray = new PlainArray();
            plainArray.remove(null);
        } catch (err) {
            const code = err.code;
            let result = (code == 401);
            map.set("test PlainArray remove0002", result == true);
        }
    }

    {
        try {
            let plainArray = new PlainArray();
            plainArray.remove(undefined);
        } catch (err) {
            const code = err.code;
            let result = (code == 401);
            map.set("test PlainArray remove0003", result == true);
        }
    }

    {
        let plainArray = new PlainArray();
        plainArray.add(1, "zhang san");
        plainArray.add(2, "zhang san1");
        let result = plainArray.remove(3);
        map.set("test PlainArray remove0004", result == undefined);
    }

    {
        let plainArray = new PlainArray();
        plainArray.add(1, "zhang san");
        plainArray.add(2, "zhang san1");
        let num = 2147483647;
        plainArray.add(num, "li si");
        let result = plainArray.remove(num);
        map.set("test PlainArray remove0005", result == "li si");
        let num1 = -2147483648;
        plainArray.add(num1, "li si");
        result = plainArray.remove(num1);
        map.set("test PlainArray remove0005", result == "li si");
    }

    {
        let plainArray = new PlainArray();
        plainArray.add(1, "squirrel");
        plainArray.add(2, "sparrow");
        let result = plainArray.removeAt(1);
        map.set("test PlainArray removeAt0001", result == "sparrow");
    }

    {
        let plainArray = new PlainArray();
        plainArray.add(1, "squirrel");
        plainArray.add(2, "sparrow");
        let result = plainArray.removeAt(-1);
        map.set("test PlainArray removeAt0002", result == undefined);
    }

    {
        let plainArray = new PlainArray();
        plainArray.add(1, "squirrel");
        plainArray.add(2, "sparrow");
        let result = plainArray.removeAt(2);
        map.set("test PlainArray removeAt0003", result == undefined);
    }

    {
        let plainArray = new PlainArray();
        let result = plainArray.removeAt(0);
        map.set("test PlainArray removeAt0004", result == undefined);
    }

    {
        try {
            let plainArray = new PlainArray();
            plainArray.add(1, "squirrel");
            plainArray.add(2, "sparrow");
            plainArray.removeRangeFrom(1, 0);
        } catch (err) {
            const code = err.code;
            let result = (code == 10200001)
            map.set("test PlainArray removeRangeFrom0001", result == true);
        }
    }

    {
        let plainArray = new PlainArray();
        plainArray.add(1, "squirrel");
        plainArray.add(2, "sparrow");
        let result = plainArray.removeRangeFrom(1, 3);
        map.set("test PlainArray removeRangeFrom0002", result == 1);
    }

    {
        let plainArray = new PlainArray();
        plainArray.add(1, "squirrel");
        plainArray.add(2, "sparrow");
        let result = plainArray.removeRangeFrom(1, Math.floor(7.5));
        map.set("test PlainArray removeRangeFrom0003", result == 1);
    }

    {
        try {
            let plainArray = new PlainArray();
            let result = plainArray.removeRangeFrom(0, 0);
        } catch (err) {
            const code = err.code;
            let result = (code == 10200001);
            map.set("test PlainArray removeRangeFrom0004", result == true);
        }
    }

    {
        let plainArray = new PlainArray();
        plainArray.add(1, "squirrel");
        plainArray.add(2, "sparrow");
        plainArray.setValueAt(1, 3546);
        let result = plainArray.getValueAt(1);
        map.set("test PlainArray setValueAt0001", result == 3546);
    }

    {
        let plainArray = new PlainArray();
        plainArray.add(1, "zhang san");
        plainArray.add(2, "zhang san1");
        let num = 2147483647;
        plainArray.add(num, "li si");
        let result = plainArray.remove(num);
        map.set("test PlainArray remove0005", result == "li si");
        let num1 = -2147483648;
        plainArray.add(num1, "li si");
        result = plainArray.remove(num1);
        map.set("test PlainArray remove0005", result == "li si");
    }

    {
        let plainArray = new PlainArray();
        plainArray.add(1, "squirrel");
        plainArray.add(2, "sparrow");
        let result = plainArray.removeAt(1);
        map.set("test PlainArray removeAt0001", result == "sparrow");
    }

    {
        let plainArray = new PlainArray();
        plainArray.add(1, "squirrel");
        plainArray.add(2, "sparrow");
        let result = plainArray.removeAt(-1);
        map.set("test PlainArray removeAt0002", result == undefined);
    }

    {
        let plainArray = new PlainArray();
        plainArray.add(1, "squirrel");
        plainArray.add(2, "sparrow");
        let result = plainArray.removeAt(2);
        map.set("test PlainArray removeAt0003", result == undefined);
    }

    {
        let plainArray = new PlainArray();
        let result = plainArray.removeAt(0);
        map.set("test PlainArray removeAt0004", result == undefined);
    }

    {
        try {
            let plainArray = new PlainArray();
            plainArray.add(1, "squirrel");
            plainArray.add(2, "sparrow");
            plainArray.removeRangeFrom(1, 0);
        } catch (err) {
            const code = err.code;
            let result = (code == 10200001)
            map.set("test PlainArray removeRangeFrom0001", result == true);
        }
    }

    {
        let plainArray = new PlainArray();
        plainArray.add(1, "squirrel");
        plainArray.add(2, "sparrow");
        let result = plainArray.removeRangeFrom(1, 3);
        map.set("test PlainArray removeRangeFrom0002", result == 1);
    }

    {
        let plainArray = new PlainArray();
        plainArray.add(1, "squirrel");
        plainArray.add(2, "sparrow");
        let result = plainArray.removeRangeFrom(1, Math.floor(7.5));
        map.set("test PlainArray removeRangeFrom0003", result == 1);
    }

    {
        try {
            let plainArray = new PlainArray();
            let result = plainArray.removeRangeFrom(0, 0);
        } catch (err) {
            const code = err.code;
            let result = (code == 10200001);
            map.set("test PlainArray removeRangeFrom0004", result == true);
        }
    }

    {
        let plainArray = new PlainArray();
        plainArray.add(1, "squirrel");
        plainArray.add(2, "sparrow");
        plainArray.setValueAt(1, 3546);
        let result = plainArray.getValueAt(1);
        map.set("test PlainArray setValueAt0001", result == 3546);
    }

    {
        let plainArray = new PlainArray();
        plainArray.add(1, "zhang san");
        plainArray.add(2, "zhang san1");
        let num = 2147483647;
        plainArray.add(num, "li si");
        let result = plainArray.remove(num);
        map.set("test PlainArray remove0005", result == "li si");
        let num1 = -2147483648;
        plainArray.add(num1, "li si");
        result = plainArray.remove(num1);
        map.set("test PlainArray remove0005", result == "li si");
    }

    {
        let plainArray = new PlainArray();
        plainArray.add(1, "squirrel");
        plainArray.add(2, "sparrow");
        let result = plainArray.removeAt(1);
        map.set("test PlainArray removeAt0001", result == "sparrow");
    }

    {
        let plainArray = new PlainArray();
        plainArray.add(1, "squirrel");
        plainArray.add(2, "sparrow");
        let result = plainArray.removeAt(-1);
        map.set("test PlainArray removeAt0002", result == undefined);
    }

    {
        let plainArray = new PlainArray();
        plainArray.add(1, "squirrel");
        plainArray.add(2, "sparrow");
        let result = plainArray.removeAt(2);
        map.set("test PlainArray removeAt0003", result == undefined);
    }

    {
        let plainArray = new PlainArray();
        let result = plainArray.removeAt(0);
        map.set("test PlainArray removeAt0004", result == undefined);
    }

    {
        try {
            let plainArray = new PlainArray();
            plainArray.add(1, "squirrel");
            plainArray.add(2, "sparrow");
            plainArray.removeRangeFrom(1, 0);
        } catch (err) {
            const code = err.code;
            let result = (code == 10200001)
            map.set("test PlainArray removeRangeFrom0001", result == true);
        }
    }

    {
        let plainArray = new PlainArray();
        plainArray.add(1, "squirrel");
        plainArray.add(2, "sparrow");
        let result = plainArray.removeRangeFrom(1, 3);
        map.set("test PlainArray removeRangeFrom0002", result == 1);
    }

    {
        let plainArray = new PlainArray();
        plainArray.add(1, "squirrel");
        plainArray.add(2, "sparrow");
        let result = plainArray.removeRangeFrom(1, Math.floor(7.5));
        map.set("test PlainArray removeRangeFrom0003", result == 1);
    }

    {
        try {
            let plainArray = new PlainArray();
            let result = plainArray.removeRangeFrom(0, 0);
        } catch (err) {
            const code = err.code;
            let result = (code == 10200001);
            map.set("test PlainArray removeRangeFrom0004", result == true);
        }
    }

    {
        let plainArray = new PlainArray();
        plainArray.add(1, "squirrel");
        plainArray.add(2, "sparrow");
        plainArray.setValueAt(1, 3546);
        let result = plainArray.getValueAt(1);
        map.set("test PlainArray setValueAt0001", result == 3546);
    }

    {
        let plainArray = new PlainArray();
        for (let i = 0; i < 1024; i++) {
          plainArray.add(i, i);
        }
        plainArray.toString();
        let result = plainArray.length;
        map.set("test PlainArray toString0006", result == 1024);
    }

    {
        let plainArray = new PlainArray();
        plainArray.add(1, "squirrel");
        plainArray.add(2, "sparrow");
        plainArray.clear();
        let result = plainArray.length;
        map.set("test PlainArray clear0001", result == 0);
    }

    {
        let plainArray = new PlainArray();
        plainArray.add(1, "squirrel");
        plainArray.clear();
        plainArray.add(2, "sparrow");
        let result = plainArray.length;
        map.set("test PlainArray clear0002", result == 1);
    }

    {
        let plainArray = new PlainArray();
        plainArray.clear();
        let result = plainArray.length;
        map.set("test PlainArray clear0003", result == 0);
    }

    {
        let plainArray = new PlainArray();
        plainArray.add(1, "squirrel");
        plainArray.add(2, "sparrow");
        let myFlag = true;
        plainArray.forEach((value, index) => {
          if (plainArray.get(index) != value) {
            myFlag = false;
          }
        });
        map.set("test PlainArray forEach0001", myFlag);
    }

    {
        let plainArray = new PlainArray();
        plainArray.add(1, "squirrel");
        plainArray.add(2, "sparrow");
        plainArray.add(3, "sparrow");
        let myFlag = true;
        plainArray.forEach((value, index) => {
            if (plainArray.get(index) != value) {
                myFlag = false;
            }
        });
        map.set("test PlainArray forEach0002", myFlag);
    }

    {
        let plainArray = new PlainArray();
        let myFlag = true;
        plainArray.forEach((value, index) => {
            if (plainArray.get(index) != value) {
                myFlag = false;
            }
        });
        map.set("test PlainArray forEach0003", myFlag);
    }

    {
        let plainArray = new PlainArray();
        plainArray.add(0, undefined);
        plainArray.add(1, null);
        let myFlag = true;
        plainArray.forEach((value, index) => {
            if (plainArray.get(index) != value) {
                myFlag = false;
            }
        });
        map.set("test PlainArray forEach0004", myFlag);
    }

    {
        let plainArray = new PlainArray();
        plainArray.add(0, undefined);
        plainArray.add(1, null);
        let myFlag = true;
        plainArray.forEach(() => {
          if (plainArray.length != 2) {
            myFlag = false;
          }
        });
        map.set("test PlainArray forEach0005", myFlag);
    }

    {
        let plainArray = new PlainArray();
        plainArray.add(1, "squirrel");
        plainArray.add(2, "sparrow");
        plainArray.add(3, "sparrow");
        let myFlag = true;
        plainArray.forEach((value, index) => {
            if (plainArray.get(index) != value) {
                myFlag = false;
            }
        });
        map.set("test PlainArray forEach0002", myFlag);
    }

    {
        let plainArray = new PlainArray();
        let myFlag = true;
        plainArray.forEach((value, index) => {
            if (plainArray.get(index) != value) {
                myFlag = false;
            }
        });
        map.set("test PlainArray forEach0003", myFlag);
    }

    {
        let plainArray = new PlainArray();
        plainArray.add(0, undefined);
        plainArray.add(1, null);
        let myFlag = true;
        plainArray.forEach((value, index) => {
            if (plainArray.get(index) != value) {
                myFlag = false;
            }
        });
        map.set("test PlainArray forEach0004", myFlag);
    }

    {
        let plainArray = new PlainArray();
        plainArray.add(0, undefined);
        plainArray.add(1, null);
        let myFlag = true;
        plainArray.forEach(() => {
          if (plainArray.length != 2) {
            myFlag = false;
          }
        });
        map.set("test PlainArray forEach0005", myFlag);
    }

    {
        let plainArray = new PlainArray();
        plainArray.add(1, "squirrel");
        plainArray.add(2, "sparrow");
        plainArray.add(3, "sparrow");
        let myFlag = true;
        plainArray.forEach((value, index) => {
            if (plainArray.get(index) != value) {
                myFlag = false;
            }
        });
        map.set("test PlainArray forEach0002", myFlag);
    }

    {
        let plainArray = new PlainArray();
        let myFlag = true;
        plainArray.forEach((value, index) => {
            if (plainArray.get(index) != value) {
                myFlag = false;
            }
        });
        map.set("test PlainArray forEach0003", myFlag);
    }

    {
        let plainArray = new PlainArray();
        plainArray.add(0, undefined);
        plainArray.add(1, null);
        let myFlag = true;
        plainArray.forEach((value, index) => {
            if (plainArray.get(index) != value) {
                myFlag = false;
            }
        });
        map.set("test PlainArray forEach0004", myFlag);
    }

    {
        let plainArray = new PlainArray();
        plainArray.add(0, undefined);
        plainArray.add(1, null);
        let myFlag = true;
        plainArray.forEach(() => {
          if (plainArray.length != 2) {
            myFlag = false;
          }
        });
        map.set("test PlainArray forEach0005", myFlag);
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
