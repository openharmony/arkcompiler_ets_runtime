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
 * @tc.require: issue#I93TZC
 */

// @ts-nocheck
declare function print(str: any): string;

class NormalClass {
    public num: number = 0;
    constructor(num: number) {
        this.num = num;
    }
}

function jsonNumber() {
    print("Start jsonNumber")
    let numValue = new SharedJSONNumber(1);
    print("jsonNumber: " + numValue.get());
    print("instanceof: " + (numValue instanceof SharedJSONNumber));
}

function jsonNull() {
    print("Start jsonNull")
    let nullValue = new SharedJSONNull();
    print("jsonNull: " + nullValue.get());
}

function jsonTrue() {
    print("Start jsonTrue")
    let trueValue = new SharedJSONTrue();
    print("jsonTrue: " + trueValue.get());
}

function jsonFalse() {
    print("Start jsonFalse")
    let falseValue = new SharedJSONFalse();
    print("jsonFalse: " + falseValue.get());
}

function jsonString() {
    print("Start jsonString")
    let stringValue = new SharedJSONString("Hello World");
    print("jsonString: " + stringValue.get());
}

function jsonArray() {
    print("Start jsonArray")
    let array = new SharedArray(new SharedJSONNumber(1), new SharedJSONNumber(3), new SharedJSONNumber(5));
    let arrValue = new SharedJSONArray(array);
    print("jsonArray: " + arrValue.get());
}

function jsonObject() {
    print("Start jsonObject")
    let numValue = new SharedJSONNumber(1024);
    let map = new SharedMap();
    map.set("hi", numValue);
    print("map: " + map);

    let jsonObj = new SharedJSONObject(map);
    print("instanceof Obj: " + (jsonObj instanceof SharedJSONObject));
    print("instanceof Null: " + (jsonObj instanceof SharedJSONNull));
    print("jsonObject: " + jsonObj.get());
}

function parseNull() {
    print("Start parseNull")
    let parse_null = SENDABLE_JSON.parse("null");
    print("parse_null:" + parse_null);
    print("parse_null:" + parse_null.get());
}

function parseFalse() {
    print("Start parseFalse")
    let parse_false = SENDABLE_JSON.parse("false");
    print("parse_false:" + parse_false);
    print("parse_false:" + parse_false.get());
}

function parseTrue() {
    print("Start parseTrue")
    let parse_true = SENDABLE_JSON.parse("true");
    print("parse_true:" + parse_true);
    print("parse_true:" + parse_true.get());
}

function parseNumber() {
    print("Start parseNumber")
    let parse_number = SENDABLE_JSON.parse("11");
    print("parse_number:" + parse_number);
    print("parse_number:" + parse_number.get());
}

function parseString() {
    print("Start parseString")
    let json_string = JSON.stringify("str");
    let parse_string = SENDABLE_JSON.parse(json_string);
    print("parse_string:" + parse_string);
    print("parse_string:" + parse_string.get());
}

function parseArray() {
    print("Start parseArray")
    let arr = [1,2,3,4,5];
    let array_string = JSON.stringify(arr);
    let parse_array = SENDABLE_JSON.parse(array_string);
    print("parse_array:" + parse_array.get());
    print("parse_array:" + parse_array.get()[2]);
    print("parse_array:" + parse_array.get()[2].get());
}

function parseObject() {
    print("Start parseObject")
    let obj = {
        "innerEntry": {
            "x": 1,
            "y": "abc",
            "str": "innerStr",
        },
        "arr": [1, 2, 3, 4, 5]
    }
    
    let parse_object = SENDABLE_JSON.parse(JSON.stringify(obj));
    print("parse_object:" + parse_object.get());
    print("parse_object:" + parse_object.get().get("arr").get());
    print("parse_object:" + parse_object.get().get("innerEntry").get().get("x"));
    print("parse_object:" + parse_object.get().get("innerEntry").get().get("x").get());
    print("parse_object:" + parse_object.get().get("innerEntry").get().get("str"));
    print("parse_object:" + parse_object.get().get("innerEntry").get().get("str").get());
    print("parse_object:" + parse_object.get().get("arr").get()[2]);
    print("parse_object:" + parse_object.get().get("arr").get()[2].get());
}

function stringifyJSONNull() {
    print("Start stringifyJSONNull");
    let nullValue = new SharedJSONNull();
    let nullString = SENDABLE_JSON.stringify(nullValue);
    print("nullString: " + nullString);
}

function stringifyJSONFalse() {
    print("Start stringifyJSONFalse");
    let falseValue = new SharedJSONFalse();
    let falseString = SENDABLE_JSON.stringify(falseValue);
    print("falseString: " + falseString);
}

function stringifyJSONTrue() {
    print("Start stringifyJSONTrue");
    let trueValue = new SharedJSONTrue();
    let trueString = SENDABLE_JSON.stringify(trueValue);
    print("trueString: " + trueString);
}

function stringifyJSONNumber() {
    print("Start stringifyJSONNumber");
    let numberValue = new SharedJSONNumber(1);
    let numberString = SENDABLE_JSON.stringify(numberValue);
    print("numberString: " + numberString);
}

function stringifyJSONString() {
    print("Start stringifyJSONString");
    let stringValue = new SharedJSONString("Hello World");
    let strString = SENDABLE_JSON.stringify(stringValue);
    print("strString: " + strString);
}

function stringifyJSONArray() {
    print("Start stringifyJSONArray");
    let array = new SharedArray(new SharedJSONNumber(1), new SharedJSONNumber(3), new SharedJSONNumber(5));
    let arrayValue = new SharedJSONArray(array);
    let arrayString = SENDABLE_JSON.stringify(arrayValue);
    print("arrayString: " + arrayString);
}

function stringifyJSONObject() {
    print("Start stringifyJSONObject");
    let numValue = new SharedJSONNumber(1024);
    let map = new SharedMap();
    map.set("hi", numValue);

    let jsonObj = new SharedJSONObject(map);
    let objString = SENDABLE_JSON.stringify(jsonObj);
    print("objString: " + objString);
}

function testJSONNull()
{
    print("Start testJSONNull");
    let nullValue = new SharedJSONNull();
    let nullString = SENDABLE_JSON.stringify(nullValue);
    print("nullString: " + nullString);
    let nullValue1 = SENDABLE_JSON.parse(nullString);
    let nullString1 = SENDABLE_JSON.stringify(nullValue1);
    print("nullString1: " + nullString1);
}

function testJSONFalse()
{
    print("Start testJSONFalse");
    let falseValue = new SharedJSONFalse();
    let falseString = SENDABLE_JSON.stringify(falseValue);
    print("falseString: " + falseString);
    let falseValue1 = SENDABLE_JSON.parse(falseString);
    let falseString1 = SENDABLE_JSON.stringify(falseValue1);
    print("falseString1: " + falseString1);
}

function testJSONTrue()
{
    print("Start testJSONTrue");
    let trueValue = new SharedJSONTrue();
    let trueString = SENDABLE_JSON.stringify(trueValue);
    print("trueString: " + trueString);
    let trueValue1 = SENDABLE_JSON.parse(trueString);
    let trueString1 = SENDABLE_JSON.stringify(trueValue1);
    print("trueString1: " + trueString1);
}

function testJSONNumber()
{
    print("Start testJSONNumber");
    let numberValue = new SharedJSONNumber(1);
    let numberString = SENDABLE_JSON.stringify(numberValue);
    print("numberString: " + numberString);
    let numberValue1 = SENDABLE_JSON.parse(numberString);
    let numberString1 = SENDABLE_JSON.stringify(numberValue1);
    print("numberString1: " + numberString1);
}

function testJSONString()
{
    print("Start testJSONString");
    let strValue = new SharedJSONString("Hello World");
    let strString = SENDABLE_JSON.stringify(strValue);
    print("strString: " + strString);
    let strValue1 = SENDABLE_JSON.parse(strString);
    let strString1 = SENDABLE_JSON.stringify(strValue1);
    print("strString1: " + strString1);
}

function testJSONArray()
{
    print("Start testJSONArray");
    let array = new SharedArray(new SharedJSONNumber(1), new SharedJSONNumber(3), new SharedJSONNumber(5));
    let arrayValue = new SharedJSONArray(array);
    let arrayString = SENDABLE_JSON.stringify(arrayValue);
    print("arrayString: " + arrayString);
    let arrayValue1 = SENDABLE_JSON.parse(arrayString);
    let arrayString1 = SENDABLE_JSON.stringify(arrayValue1);
    print("arrayString1: " + arrayString1);
}

function testJSONObject()
{
    print("Start testJSONObject");
    let numValue = new SharedJSONNumber(1024);
    let map = new SharedMap();
    map.set("hi", numValue);
    map.set("time", 2024);

    let objValue = new SharedJSONObject(map);
    let objString = SENDABLE_JSON.stringify(objValue);
    print("objString: " + objString);
    let objValue1 = SENDABLE_JSON.parse(objString);
    let objString1 = SENDABLE_JSON.stringify(objValue1);
    print("objString1: " + objString1);
}

function directCallConstructor() {
    print("Start directCallConstructor");
    try {
        SharedJSONNull();
        print("direct call SharedJSONNull ctor success.");
    } catch (err) {
        print("direct call SharedJSONNull ctor fail. err: " + err + ", errCode: " + err.code);
    }

    try {
        SharedJSONFalse();
        print("direct call SharedJSONFalse ctor success.");
    } catch (err) {
        print("direct call SharedJSONFalse ctor fail. err: " + err + ", errCode: " + err.code);
    }

    try {
        SharedJSONTrue();
        print("direct call SharedJSONTrue ctor success.");
    } catch (err) {
        print("direct call SharedJSONTrue ctor fail. err: " + err + ", errCode: " + err.code);
    }

    try {
        SharedJSONNumber(1);
        print("direct call SharedJSONNumber ctor success.");
    } catch (err) {
        print("direct call SharedJSONNumber ctor fail. err: " + err + ", errCode: " + err.code);
    }

    try {
        SharedJSONString("Hello World");
        print("direct call SharedJSONString ctor success.");
    } catch (err) {
        print("direct call SharedJSONString ctor fail. err: " + err + ", errCode: " + err.code);
    }

    try {
        let array = new SharedArray(new SharedJSONNumber(1), new SharedJSONNumber(3), new SharedJSONNumber(5));
        SharedJSONArray(array);
        print("direct call SharedJSONArray ctor success.");
    } catch (err) {
        print("direct call SharedJSONArray ctor fail. err: " + err + ", errCode: " + err.code);
    }

    try {
        let numValue = new SharedJSONNumber(1024);
        let map = new SharedMap();
        map.set("hi", numValue);
        SharedJSONObject(map);
        print("direct call SharedJSONObject ctor success.");
    } catch (err) {
        print("direct call SharedJSONObject ctor fail. err: " + err + ", errCode: " + err.code);
    }
}

function paramErrorTest() {
    print("Start paramErrorTest");
    try {
        let numValue = new SharedJSONNumber("str");
        print("new SharedJSONNumber success.");
    } catch (err) {
        print("new SharedJSONNumber fail. err: " + err + ", errCode: " + err.code);
    }

    try {
        let strValue = new SharedJSONString(111);
        print("new SharedJSONString success.");
    } catch (err) {
        print("new SharedJSONString fail. err: " + err + ", errCode: " + err.code);
    }

    try {
        let array = [1,2,3,4]
        let arrValue = new SharedJSONArray(array);
        print("new SharedJSONArray success.");
    } catch (err) {
        print("new SharedJSONArray fail. err: " + err + ", errCode: " + err.code);
    }

    try {
        let obj = {
            "innerEntry": {
                "x": 1,
                "y": "abc",
                "str": "innerStr",
            },
            "arr": [1, 2, 3, 4, 5]
        }
        let arrValue = new SharedJSONObject(obj);
        print("new SharedJSONObject success.");
    } catch (err) {
        print("new SharedJSONObject fail. err: " + err + ", errCode: " + err.code);
    }

    try {
        let obj = {
            "innerEntry": {
                "x": 1,
                "y": "abc",
                "str": "innerStr",
            },
            "arr": [1, 2, 3, 4, 5]
        }
        let str = SENDABLE_JSON.stringify(obj);
        print("SENDABLE_JSON stringify success.");
    } catch (err) {
        print("SENDABLE_JSON stringify fail. err: " + err + ", errCode: " + err.code);
    }

    try {
        let res = SENDABLE_JSON.parse("str");
        print("SENDABLE_JSON parse success.");
    } catch (err) {
        print("SENDABLE_JSON parse fail. err: " + err);
    }
}

function bindErrorTest() {
    print("Start bindErrorTest");
    const normal = new NormalClass(10);

    const null0 = new SharedJSONNull();
    const unboundGet = null0.get;
    const boundGet = unboundGet.bind(normal);
    try {
        boundGet();
        print("Call boundGet succeed.");
    } catch (err) {
        print("Call boundGet failed. err: " + err + ", errCode: " + err.code);
    }


    const false0 = new SharedJSONFalse();
    const unboundGet1 = false0.get;
    const boundGet1 = unboundGet1.bind(normal);
    try {
        boundGet1();
        print("Call boundGet1 succeed.");
    } catch (err) {
        print("Call boundGet1 failed. err: " + err + ", errCode: " + err.code);
    }

    const true0 = new SharedJSONTrue();
    const unboundGet2 = true0.get;
    const boundGet2 = unboundGet2.bind(normal);
    try {
        boundGet2();
        print("Call boundGet2 succeed.");
    } catch (err) {
        print("Call boundGet2 failed. err: " + err + ", errCode: " + err.code);
    }

    const num0 = new SharedJSONNumber(1);
    const unboundGet3 = num0.get;
    const boundGet3 = unboundGet3.bind(normal);
    try {
        boundGet3();
        print("Call boundGet3 succeed.");
    } catch (err) {
        print("Call boundGet3 failed. err: " + err + ", errCode: " + err.code);
    }

    const str0 = new SharedJSONString("Hello world");
    const unboundGet4 = str0.get;
    const boundGet4 = unboundGet4.bind(normal);
    try {
        boundGet4();
        print("Call boundGet4 succeed.");
    } catch (err) {
        print("Call boundGet4 failed. err: " + err + ", errCode: " + err.code);
    }

    let array = new SharedArray<number>(1, 3, 5);
    const arr0 = new SharedJSONArray(array);
    const unboundGet5 = arr0.get;
    const boundGet5 = unboundGet5.bind(normal);
    try {
        boundGet5();
        print("Call boundGet5 succeed.");
    } catch (err) {
        print("Call boundGet5 failed. err: " + err + ", errCode: " + err.code);
    }

    let map = new SharedMap();
    map.set("hi", "xxx");
    const obj0 = new SharedJSONObject(map);
    const unboundGet6 = obj0.get;
    const boundGet6 = unboundGet6.bind(normal);
    try {
        boundGet6();
        print("Call boundGet6 succeed.");
    } catch (err) {
        print("Call boundGet6 failed. err: " + err + ", errCode: " + err.code);
    }
}

function jsonParse() {
    parseNull();
    parseFalse();
    parseTrue();
    parseNumber();
    parseString();
    parseArray();
    parseObject();
}

function jsonStringify() {
    stringifyJSONNull();
    stringifyJSONFalse();
    stringifyJSONTrue();
    stringifyJSONNumber();
    stringifyJSONString();
    stringifyJSONArray();
    stringifyJSONObject();
}

function jsonRepeatCall() {
    testJSONNull();
    testJSONFalse();
    testJSONTrue();
    testJSONNumber();
    testJSONString();
    testJSONArray();
    testJSONObject();
}

jsonNumber()
jsonFalse()
jsonTrue()
jsonNull()
jsonString()
jsonObject()
jsonArray()
jsonParse()
jsonStringify()
jsonRepeatCall()
directCallConstructor()
paramErrorTest()
bindErrorTest()