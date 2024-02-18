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
declare function print(arg:any, arg1?:any):string;
declare class ArkTools {
    static isSlicedString(str: string): boolean;
    isAOTCompiled(args: any): boolean;
}
function assert (expr: unknown, msg?: string): asserts expr {
    if (!expr) throw new Error(msg);
}

var a : string = "12345";
var cr : string = "clashroyale";
var c = a + a.substring(2);
print(c);
a += a.substring(3);
print(a);
a = a + cr.substring(5);
print(a);

var d = cr.substring(1) + a;
print(d);

function test_undefined(message, message2, message3, message4) {
  print(message + ', ' + message2 + ', ' + message3 + ', ' + message4);
}
test_undefined('hello');

let as = "aaaaaaaa";
let bs = "bbbbbbbb";
let cs = "ccc";
let abs = as + bs;
let abcs = abs + cs;
assert(abs == "aaaaaaaabbbbbbbb", "Error optimize string add");
print(abs == "aaaaaaaabbbbbbbbccc");

// multiple add case
var Text1 = "[^-]*-";
var Text2 = Text1 + "([^-]" + Text1 + ")*-";
var Text3 = Text2 + ">?";
var Text4 = "<(!(--" + Text2 + ">|[^-]";
print(Text4);

// loop opt case
var stra = "aaaaaa";
var strb = "strb";
print(strb);
assert(ArkTools.isSlicedString(strb) == false, "Error optimize string add");
strb = stra + "bbbbbbbb"
var kk = 10;
var strc = "";
for (let i = 0; i < 3; i++) {
    if (kk > 100) {
        strc = strb + "c";
    } else {
        strc = strb + "-";
    }
}
print(strb);
let strbHasStringAddOpt: boolean = ArkTools.isSlicedString(strb);
assert(ArkTools.isSlicedString(strc) == false, "Error optimize string add");

// function call opt case
function foo1(str) {
    var strk = str + "kkk";
    for (let j = 0; j < 3; j++) {
        if (strk.length > 5) {
            var stre = str + "-";
        } else {
            var stre = str + "+";
        }
    }
}
foo1(strb);
print(strb);

// cocos app opt case
var count = 9;
function foo2(num) {
    if (num < 0) {
        return 3;
    } else {
        return "callthis";
    }
}
var right = "hmos";
if (count > 10) {
    right += foo2(count);
}
var e = "oooohhhhoooossss";
assert(ArkTools.isSlicedString(e) == false, "Error optimize string add");
for (var i = 0; i < count; i++) {
    e += right;
}
let eHasStringAddOpt: boolean = ArkTools.isSlicedString(e);

function foo3(str :string) {
    print(str);
    print(str.length);
}

let s:string = "aaaaaaaaaaaaaaaa" + "bbbbbbbbbbbbbb";
let s1:string = "aaaaaaaaaaaaaaaa" + "bbbbbbbbbbbbbb";
let ss:string = s + "-";

foo3(s);
print(s1 == s);

// two const
let str:string = "Hello World";
print("Hello" + "World");
print(str + "Hello");
print("" + "" + "123" + "");

// one const
let strs = ["OD", "Huawei", ""];
for (let i = 0; i<3; ++i) {
    let m:string = strs[i];
    print("Hello,  " + m);
}

// no const
function foo(flag) {
    let str = ["BUG", "full of bug"];
    if (flag) {
        return str[0];
    } else {
        return str[1];
    }
}
let left:string = foo(true);
let right1:string = foo(true);
let right2:string = foo(false);
print(left + right1);
print(left + right2);
print(right2 + right2);

if (ArkTools.isAOTCompiled(foo)) {
    // assert(strbHasStringAddOpt == true, "Not optimize string add");
    // assert(eHasStringAddOpt == true, "Not optimize string add");
}