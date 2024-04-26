/*
 * Copyright (c) 2023 Huawei Device Co., Ltd.
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
 * @tc.name:objectkeys
 * @tc.desc:test object keys
 * @tc.type: FUNC
 */

(function() {
    const a = {};
    Object.defineProperty(a, 'x', {
        value: 1,
        enumerable: false
    });
    let k = Object.keys(a);
    print(k.length);
    a.y = 2;
    k = Object.keys(a);
    print(k);
})();
  
(function() {
    const a = {x:1, y:2};
    let k = Object.keys(a);
    print(k[0]);
    print(k[1]);
})();

let obj = {
    a: "something",
    10: 42,
    10: 34,
    c: "string",
    d: undefined,
    d: "str",
}
print(Object.keys(obj));

(function() {
    let obj = {
        '0': { x: 12, y: 24 },
        '1000000': { x: 1, y: 2 }
    };
    print(Object.keys(obj));
    print(Object.keys(obj[0]));
    print(Object.keys(obj[1000000]));

    var o = {
        1: 1,
        2.: 2,
        3.0: 3,
        4e0: 4,
        5E0: 5,
        6e-0: 6,
        7E-0: 7,
        0x8: 8,
        0X9: 9,
    };
    let o1 = {1024: true};
    let o2 = {1024: 1024};
    print(Object.keys(o));
    print(Object.keys(o1));
    print(Object.keys(o2));
})();

const firstname = Symbol("firstname");
var object = { wode:1, wode1:2, wode2:3, wod38:4, wode4:5, wodejn5:6, wode6:7, wode7:8, wewnde8:9, wode9:10, wode10:11, wfde11:12, w32de12:13, u231mk3:14, [firstname]:"symmmm"};
print(object[firstname]);
print(object.u231mk3);
print(object.wodejn5);
print(object.wewnde8);
print(object.wode10);
print(object.u231mk3);
print(object[firstname]);
print(object.hhh);
print(object.hhh);
Object.defineProperty(object, "deffff", {
    value: object.wode4,
    writable:false,
    enumerable:true
})
print(object.deffff);
object.fff = "haode";
print(object.fff);