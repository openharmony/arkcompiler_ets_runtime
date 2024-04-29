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
 * @tc.name:String
 * @tc.desc:test String
 * @tc.type: FUNC
 * @tc.require: issueI5NO8G
 */
let tmp = '今天吃什么:告诉了会晤今天吃什么促“让今天吃什么下来好起来”今天吃今天吃什么于6月1日于北京今天吃什么面。大杂烩中国都五年来首位访华\
的今天吃什么王刚说,你吵的菜今天吃什么了赞不绝口,但是今天吃什么吃,他接到一个快递单号为#nsExpress3SF#123456789的包裹';
let flag = tmp;
let str1 = tmp.substring(0, 111) + ',,,,,,,,,,,,,,,,,,,,,,,' + tmp.substring(111 + 23);
let str2 = tmp.substring(0, 111) + 'nsExpress3SF#123456789的' + tmp.substring(111 + 23,111 + 24) + '需';
let str3 = tmp.substring(0, 111) + 'nsExpress3SF#123456789的' + tmp.substring(111 + 23);
print((flag === str1).toString());
print((flag === str2).toString());
print((flag === str3).toString());

let utf81 = '12312463347659634287446568765344234356798653455678899865746435323456709876543679334675235523463756875153764736256\
235252537585734523759078945623465357867096834523523765868';
let utf82 = utf81.substring(0, 169) + '8';
let utf83 = utf81.substring(0, 169) + '0';
print((utf81 === utf82).toString());
print((utf81 === utf83).toString());

function foo(a) {
    return a;
}
try {
    for (let i = 0; i < 25; i++) {
        foo += foo;
    }
} catch (e) {
    print(e.message);
}
let string1 = "fdjDJSAjkdfalDDGETG";
let string2 = string1.substring(1);
print(str1.charAt());
print(str2.charCodeAt());
print(str1.substr(2, 4));
print(string2.toLowerCase(2, 4));


let tmp1 = '今天吃什么:哈哈哈会晤美国phonePlatformphonePlatformphonePlatformphonePlatformphonePlatformphonePlatformphonePlatformphonePlatformphonePlatformphonePlatform国务卿促“让今天吃什么下来好起来”今天吃今天吃什么于6月1日于北京今天吃什么面。大杂烩中国都五年来首位访华\
的今天吃什么王刚说,你吵的菜今天吃什么了赞不绝口,但是今天吃什么吃,他接到一个快递单号为#nsExpress3SF#123456789的包裹';
let flag1 = tmp1;
let str = tmp1.substring(13, 143);
let str4 = tmp1.substring(13, 26);
let str5 = "phonePlatformphonePlatformphonePlatformphonePlatformphonePlatformphonePlatformphonePlatformphonePlatformphonePlatformphonePlatform";
let str6 = "phonePlatform";
var obj = {phonePlatformphonePlatformphonePlatformphonePlatformphonePlatformphonePlatformphonePlatformphonePlatformphonePlatformphonePlatform: "wode",
phonePlatform: "wode1"};

print(obj[str]);
print(obj[str4]);
print((str === str5).toString());
print((str4 == str6).toString());

var s0 = String.fromCharCode();
var s1 = String.fromCharCode(65);
var s2 = String.fromCharCode(65, 66.678);
var s3 = String.fromCharCode(0x12014);
var s4 = String.fromCharCode(true);
print(s0);
print(s1);
print(s2);
print(s3);
print(s4);

let name1 = 'testfiles就发(2).png';
let filename1 = name1.substring(0, 14);
let kk1 = filename1.toLowerCase();
print(kk1);

let name2 = 'testfileswodetwwwpng';
let filename2 = name2.substring(0, 14);
let kk2 = filename2.toLowerCase();
print(kk2);

var strA = name2 + name1;
for (let i = 0; i < 100; i++) {
strA.toLocaleString();
}
print("true");

var a = "";
var b = a.split("");
var c = a.split("1");
print(b.length);
print(c.length);
var a1 = "123_352_42342_483297538927943872943_12i2";
var a2 = "12哈哈3尼352尼42好呀342_4832975389279你们43872943尼12i2";
var a3 = a2.substring(1);
var b1 = a1.split();
var c1 = a1.split("");
var d1 = a1.split(" ");
var e1 = a1.split("_");
var f1 = a1.split("_", 2);
var g1 = a2.split("尼", 3);
var r1 = a3.split("尼", 44);
var k1 = a3.split("尼", 0);
var k2 = a3.split("4", 5);
print(b1);
print(c1);
print(d1);
print(e1);
print(f1);
print(g1);
print(r1);
print(k1.length);
print(k2);
try {
    var sym = Symbol();
    a2.split(sym, 0);
} catch (e) {
    print(e.message);
}

var test1 = "fun123ctionManualChapter123Tip1_352_42342_483297538927943872943_12i2哈哈哈";
var test2 = test1.split("_");
var test3 = test2[0];
var test4 = "1111fun123ctionManualChapter123Tip1"
let pos = test4.indexOf(test3);
print(pos);

Object.defineProperty(Array.prototype, "1", {
    value:37,
    writable:false,
});
var test5 = "123_wode2222222222222222_fast_哈哈哈";
var test6 = "123_wode2222222222222222_fast_hhh";
var test7 = test5.split("_");
var test8 = test6.split("_");
print(test7);
print(test8);

var test9 = "abc"
var test10 = new Number(4);
test10[Symbol.split] = RegExp.prototype[Symbol.split];
print(test9.split(test10));
var test11 = test1.substring(0, 31);
var test12 = "1111fun123ctionManualChapter123Tip12222"
var test13 = test12.split(test11);
var test14 = test12.lastIndexOf(test11);
print(test13);
print(test14);

let s = "abaddbdc"
s.split("")
let split = s.split("")
split[0] = "0"
print(split)
print(s.split(""))
s.split("b")
split = s.split("b")
split[2] = 1
print(split)
print(s.split("b"))
s = "d的發的給的点点滴滴多"
s.split("的")
split = s.split("的")
split[2] = 1
print(split)
print(s.split("的"))

let newStr = new String("adcdcdccda")
newStr.split("d")
split = newStr.split("d")
split[0] = "0"
print(split)
print(newStr.split("d"))

var Tag = {
    t82 : "82"
}
var tag1 = "823".substring(0, 2)
var val = "val";
var m = new Map();
m.set(tag1, val);
m.forEach((a, b)=>{
  print("Tag.t82 == b : ", Tag.t82 == b);
})

try {
  "123".padEnd(Math.pow(2, 40))
} catch (e) {
    print(e)
}

print("abc".startsWith("a", Infinity))
print("abc".startsWith("a", -Infinity))
print("abc".endsWith("c", Infinity))

const strings = [
    "ab\uD800",
    "ab\uD800c",
    "\uDFFFab",
    "c\uDFFFab",
    "abc",
    "ab\uD83D\uDE04c",
];

for (const str of strings) {
    print(str.isWellFormed());
}

const strings2 = [
    "ab\uD800",
    "ab\uD800c",
    "\uDFFFab",
    "c\uDFFFab",
    "abc",
    "ab\uD83D\uDE04c",
];

for (const str of strings2) {
    print(str.toWellFormed());
}

const text_str = "So we beat on, boats against the current, borne back ceaselessly into the past."
const search_str1 = "So we"
const search_str2 = "borne back"
print(text_str.startsWith(search_str1));
print(text_str.startsWith(search_str1, 10));
print(text_str.startsWith(search_str2, 42));
print(text_str.startsWith(search_str1, text_str.length - 1));
const text_str2 = "So we beat on, boats against the current," + " borne back ceaselessly into the past."
print(text_str2.startsWith(search_str1));
print(text_str2.startsWith(search_str1, 10));
print(text_str2.startsWith(search_str2, 42));
print(text_str2.startsWith(search_str1, text_str.length - 1));

const str7 = "测试"
const iter = str7[Symbol.iterator]();
print(iter.next().value);
print(iter.next().value);
print(iter.next().done);
print(iter.next().value);

const str_endsWith1 = 'this is a test for string endsWith!';
print(str_endsWith1.endsWith('string', '25'));    // true
print(str_endsWith1.endsWith('string', 25));      // true
print(str_endsWith1.endsWith('endsWith!', 25));   // false
print(str_endsWith1.endsWith('endsWith!'));       // true
print(str_endsWith1.endsWith('endsWith!', -1));   // false
print(str_endsWith1.endsWith('endsWith!', 100));  // true
print(str_endsWith1.endsWith('string', 25.3));    // true

let arr = Array(14);
let tree_str = arr + 512;
tree_str.substring(0,arr.length);
print("sub_string test success!");

const v0 = [3.0,-1.5053];
const v4 = v0.join();
print(v4.startsWith(v4,v0))
let strFromCodePoint = String.fromCodePoint(49,50,51,0)
print(strFromCodePoint);

{
    const v1 = [5.101952721978723,-1.2836190437943387,-1.1815850946332814e+308,2.220446049250313e-16,-680.5741773264597,0.8970365641701358,NaN];
    function f2(a3, a4, a5) {
        return BigInt64Array.bind();
    }

    const o8 = {
        "get": f2,
    };
    const v10 = new Proxy(v1, o8);
    try {
        ("n").search(v10)
    } catch (err) {
        print(err);
    }
}

// startsWith 边界值测试
const textStr = "abcd"
const matchStr = "abcd"
const offset = 0

// case offset = 0, match true
class C3 {
    constructor() {
        let bb = textStr.startsWith(matchStr, offset);
        print(bb); // true
    }
}

new C3();

// case offset = 1, match false
const offset1 = 1

class C4 {
    constructor() {
        let flag = textStr.startsWith(matchStr, offset1);
        print(flag); // false
    }
}

new C4();

print("abcd汉字".startsWith("abcd"));

{
    class C2 {
        constructor(a4) {
            a4(-3, parseFloat, ..."isFrozen");
        }
    }    

    try {
        new C2(new C2);
    } catch (err) {
        print(err);
    }
}