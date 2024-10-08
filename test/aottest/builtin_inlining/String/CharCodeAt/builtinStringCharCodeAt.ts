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
declare interface ArkTools {
  isAOTCompiled(args: any): boolean;
}
declare function print(arg:any):string;

function printCharCodeAt(str, idx) {
  try {
    print(str.charCodeAt(idx))
  } finally {
  }
}

function printCharCodeAtWithoutTry(str, idx) {
  print(str.charCodeAt(idx))
}


//aot: [trace] aot inline builtin: String.prototype.charCodeAt, caller function name:func_main_0@builtinStringCharCodeAt
//: 97
print("aaaa".charCodeAt())
//aot: [trace] aot inline builtin: String.prototype.charCodeAt, caller function name:func_main_0@builtinStringCharCodeAt
//: 97
print("aaaa".charCodeAt(1,2,3))

// Sequence String
let testLineStrUTF8 = "abcdefghijklmnopq"
let testLineStrUTF16 = "这是一个测试字符串"
//aot: [trace] aot inline builtin: String.prototype.charCodeAt, caller function name:func_main_0@builtinStringCharCodeAt
//: 98
print(testLineStrUTF8.charCodeAt(1))
//aot: [trace] aot inline builtin: String.prototype.charCodeAt, caller function name:func_main_0@builtinStringCharCodeAt
//: NaN
print(testLineStrUTF8.charCodeAt(-1))
//aot: [trace] aot inline builtin: String.prototype.charCodeAt, caller function name:func_main_0@builtinStringCharCodeAt
//: NaN
print(testLineStrUTF8.charCodeAt(testLineStrUTF8.length))
//aot: [trace] aot inline builtin: String.prototype.charCodeAt, caller function name:func_main_0@builtinStringCharCodeAt
//: 26159
print(testLineStrUTF16.charCodeAt(1))
//aot: [trace] aot inline builtin: String.prototype.charCodeAt, caller function name:func_main_0@builtinStringCharCodeAt
//: NaN
print(testLineStrUTF16.charCodeAt(-1))
//aot: [trace] aot inline builtin: String.prototype.charCodeAt, caller function name:func_main_0@builtinStringCharCodeAt
//: NaN
print(testLineStrUTF16.charCodeAt(testLineStrUTF16.length))

// Slice String
let slicedStringUTF8 = "This is a segment of sliced string, please test charcodeat".slice(15, 30);
let slicedStringUTF16 = "这是一段Sliced String, 请用这段文字来测试charcodeatxxxxxxxxxxxxxx".slice(10, 30);
//aot: [trace] aot inline builtin: String.prototype.charCodeAt, caller function name:func_main_0@builtinStringCharCodeAt
//: 102
print(slicedStringUTF8.charCodeAt(4))
//aot: [trace] aot inline builtin: String.prototype.charCodeAt, caller function name:func_main_0@builtinStringCharCodeAt
//: NaN
print(slicedStringUTF8.charCodeAt(-1))
//aot: [trace] aot inline builtin: String.prototype.charCodeAt, caller function name:func_main_0@builtinStringCharCodeAt
//: NaN
print(slicedStringUTF8.charCodeAt(slicedStringUTF8.length))
//aot: [trace] aot inline builtin: String.prototype.charCodeAt, caller function name:func_main_0@builtinStringCharCodeAt
//: 105
print(slicedStringUTF16.charCodeAt(4))
//aot: [trace] aot inline builtin: String.prototype.charCodeAt, caller function name:func_main_0@builtinStringCharCodeAt
//: NaN
print(slicedStringUTF16.charCodeAt(-1))
//aot: [trace] aot inline builtin: String.prototype.charCodeAt, caller function name:func_main_0@builtinStringCharCodeAt
//: NaN
print(slicedStringUTF16.charCodeAt(slicedStringUTF16.length))

// Tree String
let treeStringUTF8 = "This is tree String part A" + "This is tree String part B";
let treeStringUTF16 = "这是树形字符串的第一个部分" + "这是树形字符串的第二个部分";
// Check path for tree string is not flat
//aot: [trace] aot inline builtin: String.prototype.charCodeAt, caller function name:func_main_0@builtinStringCharCodeAt
//: 32
print(treeStringUTF8.charCodeAt(4))
// Check path for tree string is flat
//aot: [trace] aot inline builtin: String.prototype.charCodeAt, caller function name:func_main_0@builtinStringCharCodeAt
//: 115
print(treeStringUTF8.charCodeAt(6))
//aot: [trace] aot inline builtin: String.prototype.charCodeAt, caller function name:func_main_0@builtinStringCharCodeAt
//: NaN
print(treeStringUTF8.charCodeAt(-1))
//aot: [trace] aot inline builtin: String.prototype.charCodeAt, caller function name:func_main_0@builtinStringCharCodeAt
//: NaN
print(treeStringUTF8.charCodeAt(treeStringUTF8.length))
//aot: [trace] aot inline builtin: String.prototype.charCodeAt, caller function name:func_main_0@builtinStringCharCodeAt
//: 23383
print(treeStringUTF16.charCodeAt(4))
// Check path for tree string is flat
//aot: [trace] aot inline builtin: String.prototype.charCodeAt, caller function name:func_main_0@builtinStringCharCodeAt
//: 20018
print(treeStringUTF16.charCodeAt(6))
//aot: [trace] aot inline builtin: String.prototype.charCodeAt, caller function name:func_main_0@builtinStringCharCodeAt
//: NaN
print(treeStringUTF16.charCodeAt(-1))
//aot: [trace] aot inline builtin: String.prototype.charCodeAt, caller function name:func_main_0@builtinStringCharCodeAt
//: NaN
print(treeStringUTF16.charCodeAt(treeStringUTF16.length))


//aot: [trace] aot inline builtin: String.prototype.charCodeAt, caller function name:#*#printCharCodeAt@builtinStringCharCodeAt
//aot: [trace] Check Type: NotInt1
//: 97
printCharCodeAt("aaaa", "1")

let badString = {}
badString.charCodeAt = String.prototype.charCodeAt
//aot: [trace] Check Type: NotString1
//: 111
printCharCodeAt(badString, 1)

function replace(i)
{
  return i;
}
let testStr = "abcdefg";
// Replace standard builtin
let true_charCodeAt = String.prototype.charCodeAt
if (ArkTools.isAOTCompiled(printCharCodeAt)) {
  String.prototype.charCodeAt = replace
}

//aot: [trace] Check Type: NotCallTarget1
//aot: 1
//pgo: 98
printCharCodeAt(testStr, 1)

String.prototype.charCodeAt = true_charCodeAt

//aot: [trace] aot inline builtin: String.prototype.charCodeAt, caller function name:func_main_0@builtinStringCharCodeAt
//: 97
print("aaa".charCodeAt(1))

//aot: [trace] aot inline function name: #*#printCharCodeAtWithoutTry@builtinStringCharCodeAt caller function name: func_main_0@builtinStringCharCodeAt
//aot: [trace] aot inline builtin: String.prototype.charCodeAt, caller function name:#*#printCharCodeAtWithoutTry@builtinStringCharCodeAt
//: 97
printCharCodeAtWithoutTry("aaa", 1)

// Test for AOT only
if (ArkTools.isAOTCompiled(printCharCodeAt)) {
  String.prototype.charCodeAt = function() {
    throw new Error("custom Error")
  }
  try {
    printCharCodeAtWithoutTry(testStr, 1);
  } catch {
    //aot: [trace] Check Type: NotCallTarget1
    //aot: Exception In AOT1
    print("Exception In AOT1")
  }

  String.prototype.charCodeAt = true_charCodeAt
  let symbol = Symbol("aaa")
  try {
    printCharCodeAtWithoutTry(symbol, 1);
  } catch {
    //aot: [trace] Check Type: BuiltinInstanceHClassMismatch2
    //aot: Exception In AOT2
    print("Exception In AOT2")
  }

  let strNull = null
  try {
    printCharCodeAtWithoutTry(strNull, 0)
  } catch {
    //aot: [trace] Check Type: NotHeapObject1
    //aot: Exception In AOT3
    print("Exception In AOT3")
  }

  let strUndefine;
  try {
    printCharCodeAtWithoutTry(strUndefine, 0)
  } catch {
    //aot: [trace] Check Type: NotHeapObject1
    //aot: Exception In AOT4
    print("Exception In AOT4")
  }

  let strTest = "hello";
  try {
    printCharCodeAtWithoutTry(strTest, { toString: function() { throw new Error("Error"); } })
  } catch {
    //aot: [trace] aot inline builtin: String.prototype.charCodeAt, caller function name:#*#printCharCodeAtWithoutTry@builtinStringCharCodeAt
    //aot: [trace] Check Type: NotInt1
    //aot: Exception In AOT5
    print("Exception In AOT5")
  }
}
