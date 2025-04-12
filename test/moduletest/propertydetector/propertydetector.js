/*
 * Copyright (c) 2023 Shenzhen Kaihong Digital Industry Development Co., Ltd.
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
 * @tc.name:property detector
 * @tc.desc:test property detector
 * @tc.type: FUNC
 * @tc.require: issueI83B0E
 */

print(ArkTools.isRegExpFlagsDetectorValid())
Object.defineProperty(RegExp.prototype, 'flags', {
  get: function () {
    return undefined;
  }
});
print(ArkTools.isRegExpFlagsDetectorValid())

let str = '这是一段原始文本,"3c这要替换4d"!';
let regexp = /([0-9])([a-z])/g
let newStr1 = str.replace(regexp, "$1" );
print(newStr1)
print(ArkTools.isRegExpReplaceDetectorValid())

let p = RegExp.prototype
let unused = p[Symbol.replace]
print(ArkTools.isRegExpReplaceDetectorValid())

p["replace"] = function () {return "abc"}
print(ArkTools.isRegExpReplaceDetectorValid())

regexp.__proto__ = {}
let newStr2 = str.replace(regexp, "$2" );
print(newStr2)
print(ArkTools.isRegExpReplaceDetectorValid())

// resume regexp.__proto__
regexp.__proto__ = p
p[Symbol.replace] = function () {return "aaa"}
let newStr3 = str.replace(regexp, "$3" );
print(newStr3)
print(ArkTools.isRegExpReplaceDetectorValid())

p[Symbol.replace] = function () {return 4}
let newStr4 = str.replace( /([0-9])([a-z])/g,"$4" );
print(newStr4)
print(ArkTools.isRegExpReplaceDetectorValid())

print(ArkTools.isNumberStringNotRegexpLikeDetectorValid());
String.prototype[Symbol.matchAll] = function () {return "aaa"}
print(ArkTools.isNumberStringNotRegexpLikeDetectorValid());

for (let i = 0; i < 15; i++) {
  let __proto__ = ['a', 'b'];
  0x3fffffff.__proto__.__proto__ = [1, 2, 3];
  __proto__.var06 = 1;
}
print("RegisterOnProtoChain success");
