/*
 * Copyright (c) 2024 Huawei Device Co., Ltd.
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
 * @tc.name:sharedJSON
 * @tc.desc:test sharedJSON
 * @tc.type: FUNC
 * @tc.require: issueI9K9KB
 */

// @ts-nocheck
declare function print(str: any): string;

const enum BigIntMode {
    DEFAULT = 0,
    PARSE_AS_BIGINT = 1,
    ALWAYS_PARSE_AS_BIGINT = 2,
};

const enum ParseReturnType {
    OBJECT = 0,
    MAP = 1,
};

let input = '{"big":1122334455667788999,"small":123,"deci":1234567890.0123456,"shortExp":1.79e+308,"longExp":1.7976931348623157e+308}';
let input2 = '{"zerodeci1":0.0000123,"zerodeci2":0.4799123,"zerodeci3":0.7777334477383838389929292922,"zerodeci4":0.0000000000000123}';

let obj = {
    "innerEntry": {
        "x": 1,
        "y": "abc",
        "str": "innerStr",
    },
    "arr": [1, 2, 3, 4, 5]
}

let arr = [1, 3, 5, 7, 9];

let obj1 = {
    "x": 1,
    "y": "你好"
}

function testJSONParseSendable() {
    print("Start testJSONParseSendable")
    let str = JSON.stringify(obj);
    let sharedObj = JSON.parseSendable(str);
    print("sharedObj.arr: " + sharedObj.arr);
    print("sharedObj.innerEntry: " + sharedObj.innerEntry);
    print("sharedObj.innerEntry.x: " + sharedObj.innerEntry.x);
    print("sharedObj.innerEntry.y: " + sharedObj.innerEntry.y);
    print("sharedObj.innerEntry.str: " + sharedObj.innerEntry.str);

    let str1 = JSON.stringify(arr);
    let sharedArr = JSON.parseSendable(str1);
    print("sharedArr: " + sharedArr);

    let str2 = JSON.stringify(obj1);
    let sharedObj1 = JSON.parseSendable(str2);
    print("sharedObj1.x: " + sharedObj1.x);
    print("sharedObj1.y: " + sharedObj1.y);
}

function jsonRepeatCall() {
    print("Start jsonRepeatCall")
    let stringify1 = JSON.stringify(obj);
    print("stringify1: " + stringify1);
    let sharedObj = JSON.parseSendable(stringify1);
    let stringify2 = JSON.stringify(sharedObj);
    print("stringify2: " + stringify2);
    let normalObj = JSON.parse(stringify2);
    let stringify3 = JSON.stringify(normalObj);
    print("stringify3: " + stringify3);

    let stringify4 = JSON.stringify(arr);
    print("stringify4: " + stringify4);
    let sharedArr = JSON.parseSendable(stringify4);
    let stringify5 = JSON.stringify(sharedArr);
    print("stringify5: " + stringify5);
    let normalArr = JSON.parse(stringify5);
    let stringify6 = JSON.stringify(normalArr);
    print("stringify6: " + stringify6);

    let stringify7 = JSON.stringify(obj1);
    print("stringify7: " + stringify7);
    let sharedObj1 = JSON.parseSendable(stringify7);
    let stringify8 = JSON.stringify(sharedObj1);
    print("stringify8: " + stringify8);
    let normalObj1 = JSON.parse(stringify8);
    let stringify9 = JSON.stringify(normalObj1);
    print("stringify9: " + stringify9);

    let strSendable = '{"x":1, "x":2, "x":3, "x":"你好", "a":4, "x":"你好", "a":2}';
    let sharedObj2 = JSON.parseSendable(strSendable);
    print("sharedObj2.x: " + sharedObj2.x);
    print("sharedObj2.a: " + sharedObj2.a);
    
    let stringify10 = JSON.stringify(sharedObj2);
    print("stringify10: " + stringify10);
}

function testASONBigInt() {
    var options1 = {
        bigIntMode: BigIntMode.PARSE_AS_BIGINT,
    }
    var obj = JSON.parseSendable(input, undefined, options1);

    print(obj.small);
    print(obj.big);
    print((typeof obj.big === "bigint"));

    var options2 = {
        bigIntMode: BigIntMode.ALWAYS_PARSE_AS_BIGINT,
    }
    var obj2 = JSON.parseSendable(input, undefined, options2);
    print(obj2.small);
    print(obj2.big);
    print((typeof obj2.small === "bigint"));

    var options3 = {
        bigIntMode: BigIntMode.PARSE_AS_BIGINT,
    }
    var obj3 = JSON.parseSendable(input, undefined, options3);
    print(obj3.deci);
    print(obj3.shortExp);
    print(obj3.longExp);

    var options4 = {
        bigIntMode: BigIntMode.PARSE_AS_BIGINT,
    }
    var obj4 = JSON.parseSendable(input, undefined, options4);
    var output = JSON.stringifySendable(obj4);
    print(output);

    var options5 = {
        bigIntMode: BigIntMode.ALWAYS_PARSE_AS_BIGINT,
    }
    var obj5 = JSON.parseSendable(input, undefined, options5);
    var output2 = JSON.stringifySendable(obj5);
    print(output2);
}

function testJSONBigInt() {
    var options1 = {
        bigIntMode: BigIntMode.PARSE_AS_BIGINT,
    }
    var obj = JSON.parseBigInt(input, undefined, options1);

    print(obj.small);
    print(obj.big);
    print((typeof obj.big === "bigint"));

    var options2 = {
        bigIntMode: BigIntMode.ALWAYS_PARSE_AS_BIGINT,
    }
    var obj2 = JSON.parseBigInt(input, undefined, options2);
    print(obj2.small);
    print(obj2.big);
    print((typeof obj2.small === "bigint"));

    var options3 = {
        bigIntMode: BigIntMode.PARSE_AS_BIGINT,
    }
    var obj3 = JSON.parseBigInt(input, undefined, options3);
    print(obj3.deci);
    print(obj3.shortExp);
    print(obj3.longExp);

    var options4 = {
        bigIntMode: BigIntMode.PARSE_AS_BIGINT,
    }
    var obj4 = JSON.parseBigInt(input, undefined, options4);
    var output = JSON.stringifyBigInt(obj4);
    print(output);

    var options5 = {
        bigIntMode: BigIntMode.ALWAYS_PARSE_AS_BIGINT,
    }
    var obj5 = JSON.parseBigInt(input, undefined, options5);
    var output2 = JSON.stringifyBigInt(obj5);
    print(output2);

}

function testJSONNormal() {
    var obj = JSON.parseBigInt(input, undefined, undefined);
    print(obj.small);
    print(obj.big);
    print(obj.deci);
    print(obj.shortExp);
    print(obj.longExp);
    print((typeof obj.small === "number"));
    print((typeof obj.big === "number"));
    print((typeof obj.deci === "number"));
    print((typeof obj.shortExp === "number"));
    print((typeof obj.longExp === "number"));
}

function testJSONreviver() {
    let opt = {
        bigIntMode: BigIntMode.ALWAYS_PARSE_AS_BIGINT,
    }
    try {
        JSON.parseSendable(input, function (k, v) {return v;});
    } catch (error) {
        print(error);
    }
    try {
        JSON.parseSendable(input, function (k, v) {return v;}, opt);
    } catch (error) {
        print(error);
    }
}

function testJSONZeroDeci() {
    let opt1 = {
        bigIntMode: BigIntMode.ALWAYS_PARSE_AS_BIGINT,
    }
    let opt2 = {
        bigIntMode: BigIntMode.PARSE_AS_BIGINT,
    }
    let obj1 = JSON.parseSendable(input2, undefined, opt1);
    print(obj1.zerodeci1);
    print(obj1.zerodeci2);
    print(obj1.zerodeci3);
    print(obj1.zerodeci4);
    let obj2 = JSON.parseSendable(input2, undefined, opt2);
    print(obj2.zerodeci1);
    print(obj2.zerodeci2);
    print(obj2.zerodeci3);
    print(obj2.zerodeci4);
    let obj3 = JSON.parseBigInt(input2, undefined, opt1);
    print(obj3.zerodeci1);
    print(obj3.zerodeci2);
    print(obj3.zerodeci3);
    print(obj3.zerodeci4);
    let obj4 = JSON.parseBigInt(input2, undefined, opt2);
    print(obj4.zerodeci1);
    print(obj4.zerodeci2);
    print(obj4.zerodeci3);
    print(obj4.zerodeci4);
}

function testASONMap() {
    let jsonText1 = '{"text":"ASON support MAP Test Start","largeNumber":112233445566778899,"people":{"name":"Mary","sex":"1","height":"165"}}';
    let options1 = {
        bigIntMode: BigIntMode.PARSE_AS_BIGINT,
        parseReturnType: ParseReturnType.MAP, 
    }
    let map = JSON.parseSendable(jsonText1, undefined, options1) as Map<string, object>;

    print(map.get("text"));
    print(map.get("largeNumber"));
    print((typeof map.get("largeNumber") === "bigint"));
    print(map.get("people"));

    let options2 = {
        bigIntMode: BigIntMode.DEFAULT,
        parseReturnType: ParseReturnType.MAP, 
    }

    let jsonText2 = '{';
    try {
        let map2 = JSON.parseSendable(jsonText2, undefined, options2) as Map<string, object>;
    } catch (error) {
        print(error);
    }

    let jsonText3 = '{"city"}';
    try {
        let map3 = JSON.parseSendable(jsonText3, undefined, options2) as Map<string, object>;
    } catch (error) {
        print(error);
    }

    let jsonText4 = '{}';
    let map4 = JSON.parseSendable(jsonText4, undefined, options2) as Map<string, object>;
    print(map4);

    let jsonText5 = '{"x":1, "x":2, "x":3, "x":"你好", "a":4, "x":"你好", "a":2}';
    let map5 = JSON.parseSendable(jsonText5, undefined, options2) as Map<string, object>;
    print("sendableMap5 size: " + map5.size);
    print("sendableMap5 x: " + map5.get("x"));
    print("sendableMap5 a: " + map5.get("a"));

    let jsonText6 = '{"arr": [1,2,3], "boolA":true, "boolB":false, "nullText":null}';
    let map6 = JSON.parseSendable(jsonText6, undefined, options2) as Map<string, object>;
    print("sendableMap6 arr: " + map6.get("arr"));
    print("sendableMap6 boolA: " + map6.get("boolA"));
    print("sendableMap6 boolB: " + map6.get("boolB"));
    print("sendableMap6 nullText: " + map6.get("nullText"));
}

function testIndexASON()
{
    let asonstr1 = '{"12":"45", "67":"89"}';
    let asonstr2 = '{"12":"45", "67":"89", "a":"b"}';
    let a = JSON.parseSendable(asonstr1);
    let b = JSON.parseSendable(asonstr2);
    print("ASON parse asonstr1: " + a["12"]);
    print("ASON parse asonstr2: " + b["67"]);
    let asonstr3 =
        '{"123":"aa", "xx":"yy", "aaa":"es", "1234":"bb", "aaa":"ee", "124":"123", "success":"true", "123":"1"}';
    let c = JSON.parseSendable(asonstr3);
    let outstr = c.aaa + c.success + c[123] + c[124];
    print("ASON parse asonstr3: " + outstr);
    let out3 = JSON.stringifySendable(c)
    print(out3);
}

function testJSONBigIntZero() {
    var opt = {
        bigIntMode: BigIntMode.ALWAYS_PARSE_AS_BIGINT,
    }
    const str = '{"id":0, "bid":999}';
    var obj = JSON.parseBigInt(str, undefined, opt);
    print(obj.id);
}

function testASONStringifyMapAndSet() {
    let jsonText1 = '{"text":"ASON support MAP Test Start","largeNumber":112233445566778899,"people":{"name":"Mary","sex":"1","height":"165"}}';
    let options1 = {
        bigIntMode: BigIntMode.PARSE_AS_BIGINT,
        parseReturnType: ParseReturnType.MAP, 
    }
    let map1 = JSON.parseSendable(jsonText1, undefined, options1);
    let output1 = JSON.stringifySendable(map1);
    print(output1);

    let jsonText2 = '{"args1":true,"args2":false,"args3":null}';
    let map2 = JSON.parseSendable(jsonText2, undefined, options1);
    let output2 = JSON.stringifySendable(map2);
    print(output2);

    let jsonText3 = '{"people":{"name":"Mary","sex":"1","height":165,"args":{"arr":[1,2,3],"check":true,"num":null}}}';
    let map3 = JSON.parseSendable(jsonText3, undefined, options1);
    let output3 = JSON.stringifySendable(map3);
    print(output3);

    let jsonText4 = '{"features":{"ut":{"args":{"bizType":"SQYK","isChecked":false,"packageId":9223372036854775806}}}}';
    let map4 = JSON.parseSendable(jsonText4, undefined, options1);
    let output4 = JSON.stringifySendable(map4);
    print(output4);

    let set = new Set(['foo', 'bar', 'baz']);
    let output5 = JSON.stringifySendable(set);
    print(output5);

    let sharedSet = new SendableSet(['foo', 'bar', 'baz']);
    let output6 = JSON.stringifySendable(sharedSet);
    print(output6);

    let map5 = new Map([['foo', 1], ['bar', 2], ['baz', 3]]);
    let output7 = JSON.stringifySendable(map5);
    print(output7);

    let arkPrivate = globalThis.ArkPrivate;
    var HashMap = arkPrivate.Load(arkPrivate.HashMap);
    let hashMap = new HashMap();
    hashMap.set('foo', 1);
    hashMap.set('bar', 2);
    hashMap.set('baz', 3);
    let output8 = JSON.stringifySendable(hashMap);
    print(output8 != '{}');

    var HashSet = arkPrivate.Load(arkPrivate.HashSet);
    let hashSet = new HashSet();
    hashSet.add('foo');
    hashSet.add('bar');
    hashSet.add('baz');
    let output9 = JSON.stringifySendable(hashSet);
    print(output9 != '{}' && output9 != '[]');
}

class SharedA {
    v: number;
    map: SendableMap<string, SendableArray<number>>;
    constructor() {
        'use sendable'
    }
}

class A {
    v: number;
    map: Map<string, Array<number>>;
    constructor() {
    }
}

function testASONStringifyMapAndSetAndObj() {
    let array1 = new SendableArray<number>(1, 2, 3, 4);
    let array2 = new SendableArray<number>(3, 5, 7, 9);
    let sharedMap = new SendableMap();
    sharedMap.set('arr1', array1);
    sharedMap.set('arr2', array2);
    let output1 = JSON.stringifySendable(sharedMap);
    let output2 = JSON.stringify(sharedMap);
    print(output1);
    print(output2);

    let sharedA = new SharedA();
    sharedA.v = 100;
    sharedA.map = sharedMap;
    let output3 = JSON.stringifySendable(sharedA);
    print(output3);
    let output4 = JSON.stringify(sharedA);
    print(output4);

    let sharedSet = new SendableSet([sharedA]);
    let output5 = JSON.stringifySendable(sharedSet);
    print(output5);
    let output6 = JSON.stringify(sharedSet);
    print(output6);

    let array3 = new Array<number>(1, 2, 3, 4);
    let array4 = new Array<number>(3, 5, 7, 9);
    let set = new Set();
    let map = new Map();
    map.set('arr3', array3);
    map.set('arr4', array4);
    let output7 = JSON.stringifySendable(map);
    print(output7);
    let output8 = JSON.stringify(map);
    print(output8);

    let a = new A();
    a.v = 200;
    a.map = map;
    let output9 = JSON.stringifySendable(a);
    print(output9);
    let output10 = JSON.stringify(a);
    print(output10);
    set.add(a);
    let output11 = JSON.stringifySendable(set);
    print(output11);
    let output12 = JSON.stringify(set);
    print(output12);

    let arkPrivate = globalThis.ArkPrivate;
    var HashMap = arkPrivate.Load(arkPrivate.HashMap);
    let hashMap = new HashMap();
    hashMap.set('set', set);
    let output13 = JSON.stringifySendable(hashMap);
    print(output13);
    let output14 = JSON.stringify(hashMap);
    print(output14);

    var HashSet = arkPrivate.Load(arkPrivate.HashSet);
    let hashSet = new HashSet();
    hashSet.add(hashMap);
    let output15 = JSON.stringifySendable(hashSet);
    print(output15);
    let output16 = JSON.stringify(hashSet);
    print(output16);
}

function testASONStringifyAfterClearMapAndSet() {
    let map1 = new Map<string | null | number | boolean, string | null | number | boolean>();
    map1.set("a1", "A1");
    map1.clear();
    map1.set(1, 1);
    map1.set(true, true);
    let str1 = JSON.stringifySendable(map1);
    print(str1);

    let map2 = new SendableMap<string | null | number | boolean, string | null | number | boolean>();
    map2.set("a1", "A1");
    map2.clear();
    map2.set(1, 1);
    map2.set(true, true);
    let str2 = JSON.stringifySendable(map2);
    print(str2);

    let set1 = new Set<string | null | number | boolean>();
    set1.add("a1");
    set1.clear();
    set1.add(1);
    set1.add(true);
    let str3 = JSON.stringifySendable(set1);
    print(str3);

    let set2 = new SendableSet<string | null | number | boolean>();
    set2.add("a1");
    set2.clear();
    set2.add(1);
    set2.add(true);
    let str4 = JSON.stringifySendable(set2);
    print(str4);
}

function testASONStringifyMapSetAddUndefined() {
    let map1 = new Map<string | null | number | boolean | undefined, string | null | number | boolean | undefined>();
    map1.set("a1", 1);
    map1.set(undefined, "a2");
    map1.set(null, undefined);
    map1.set(undefined, undefined);
    map1.set(undefined, null);
    let str1 = JSON.stringifySendable(map1);
    print(str1);

    let map2 = new SendableMap<string | null | number | boolean | undefined, string | null | number | boolean | undefined>();
    map2.set("a1", 1);
    map2.set(undefined, "a2");
    map2.set(null, undefined);
    map2.set(undefined, undefined);
    map2.set(undefined, null);
    let str2 = JSON.stringifySendable(map2);
    print(str2);

    let arkPrivate = globalThis.ArkPrivate;
    var HashMap = arkPrivate.Load(arkPrivate.HashMap);
    let hashMap = new HashMap<string | null | number | boolean | undefined, string | null | number | boolean | undefined>();
    hashMap.set("a1", 1);
    hashMap.set(null, undefined);
    let str3 = JSON.stringifySendable(hashMap);
    print(str3);

    let set1 = new Set<string | null | number | boolean | undefined>();
    set1.add("a1");
    set1.add(undefined);
    set1.add(1);
    set1.add(true);
    let str4 = JSON.stringifySendable(set1);
    print(str4);

    let set2 = new SendableSet<string | null | number | boolean | undefined>();
    set2.add("a1");
    set2.add(undefined);
    set2.add(1);
    set2.add(true);
    let str5 = JSON.stringifySendable(set2);
    print(str5);

    var HashSet = arkPrivate.Load(arkPrivate.HashSet);
    let hashSet = new HashSet();
    hashSet.add("a1");
    hashSet.add(undefined);
    hashSet.add(1);
    hashSet.add(true);
    let str6 = JSON.stringifySendable(hashSet);
    print(str6);
}

function testOverInt32Key() {
    let jsonText = '{"2300004177":{"number":"2300004177"}}';
    let obj = JSON.parseSendable(jsonText);
    let str = JSON.stringify(obj);
    print(str)
    let jsonText1 = '{"100":{"number":"100"}}';
    let obj1 = JSON.parseSendable(jsonText1);
    let str1 = JSON.stringify(obj1);
    print(str1)
}

testJSONParseSendable();
jsonRepeatCall();
testASONBigInt();
testJSONBigInt();
testJSONNormal();
testJSONreviver();
testJSONZeroDeci();
testASONMap();
testIndexASON();
testJSONBigIntZero();
testASONStringifyMapAndSet();
testASONStringifyMapAndSetAndObj();
testASONStringifyAfterClearMapAndSet();
testASONStringifyMapSetAddUndefined();
testOverInt32Key();
