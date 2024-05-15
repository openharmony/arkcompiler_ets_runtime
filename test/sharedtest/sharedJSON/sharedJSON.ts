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

}

testJSONParseSendable();
jsonRepeatCall();