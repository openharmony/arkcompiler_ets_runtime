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
let arr = ["a",2,3,4];
//aot: [trace] aot call builtin: Array.prototype.push, caller function name:func_main_0@builtinArrayPush
print(arr.push()) //: 4

function testArrayPushagrs1() {
    print("testArrayPushagrs1") //: testArrayPushagrs1
    //aot: [trace] aot inline builtin: Array.prototype.push, caller function name:#*#testArrayPushagrs1@builtinArrayPush
    let arr = ["a",2,3,4];
    let x = arr.push("s");
    print(arr.length) //: 5
    print(arr[4]) //: s
    print(x) //: 5

    let arr2 = new Array()
    //aot: [trace] aot inline builtin: Array.prototype.push, caller function name:#*#testArrayPushagrs1@builtinArrayPush
    let y = arr2.push(1);
    print(y) //: 1
}
testArrayPushagrs1();

//aot: [trace] aot inline function name: #*#testArrayPushagrs2@builtinArrayPush caller function name: func_main_0@builtinArrayPush
function testArrayPushagrs2() {
    print("testArrayPushagrs2") //: testArrayPushagrs2
    let arr = ["a",2,3,4];
    //aot: [trace] aot call builtin: Array.prototype.push, caller function name:#*#testArrayPushagrs2@builtinArrayPush
    print(arr.push(1,2)) //: 6
    print(arr[4])   //: 1
    print(arr[5])   //: 2
}
testArrayPushagrs2()
//aot: [trace] aot inline function name: #*#testArrayPushNotStableArray@builtinArrayPush caller function name: func_main_0@builtinArrayPush
function testArrayPushNotStableArray() {
    print("testArrayPushNotStableArray") //: testArrayPushNotStableArray
    let arr = new Array(1025);
    //aot: [trace] aot inline builtin: Array.prototype.push, caller function name:#*#testArrayPushNotStableArray@builtinArrayPush
    //aot: [trace] Check Type: NotStableArray2
    print(arr.push(0));  //: 1026
}
testArrayPushNotStableArray()


function testPushToNonWritableLength() {
    print("testPushToNonWritableLength") //: testPushToNonWritableLength
    var arr_push = [];
    Object.defineProperty(arr_push, "length", { writable : false});
    try {
        //aot: [trace] aot inline builtin: Array.prototype.push, caller function name:#*#testPushToNonWritableLength@builtinArrayPush
        //aot: [trace] Check Type: BuiltinInstanceHClassMismatch
        arr_push.push("5");
    } catch (e) {
        print(e instanceof TypeError); //: true
    }
}
testPushToNonWritableLength();




function testArrayPushNotStableArray4() {
    print("testArrayPushNotStableArray4") //: testArrayPushNotStableArray4
    var arr = new Array(1023)
    for(let i = 0; i < 5; i++){
        //aot: [trace] aot inline builtin: Array.prototype.push, caller function name:#*#testArrayPushNotStableArray4@builtinArrayPush
        //aot: [trace] aot inline builtin: Array.prototype.push, caller function name:#*#testArrayPushNotStableArray4@builtinArrayPush
        //aot: [trace] aot inline builtin: Array.prototype.push, caller function name:#*#testArrayPushNotStableArray4@builtinArrayPush
        //aot: [trace] aot inline builtin: Array.prototype.push, caller function name:#*#testArrayPushNotStableArray4@builtinArrayPush
        //aot: [trace] aot inline builtin: Array.prototype.push, caller function name:#*#testArrayPushNotStableArray4@builtinArrayPush
        arr.push(i);
    }
    //aot: [trace] aot call builtin: Array.prototype.push, caller function name:#*#testArrayPushNotStableArray4@builtinArrayPush
    print(arr.push()) //: 1028
}
testArrayPushNotStableArray4()



function testArrayPushException(){
    print("testArrayPushException") //: testArrayPushException
    Object.freeze(arr);
    let error = null;
    try {
      //aot: [trace] aot inline builtin: Array.prototype.push, caller function name:#*#testArrayPushException@builtinArrayPush
      //aot: [trace] Check Type: NotStableArray2
      arr.push(1);
    } catch (e) {
      error = e;
      print("exception happend") //: exception happend
    }
  }
testArrayPushException()

function testArrayPushMaxLength(){
    print("testArrayPushMaxLength") //: testArrayPushMaxLength
    let arr = [];
    //aot: [trace] aot inline builtin: Math.pow, caller function name:#*#testArrayPushMaxLength@builtinArrayPush
    arr.length = Math.pow(2, 32) - 1; // 设置数组长度为最大值
    try {
      //aot: [trace] aot inline builtin: Array.prototype.push, caller function name:#*#testArrayPushMaxLength@builtinArrayPush
      //aot: [trace] Check Type: NotStableArray2
      arr.push(1);
    } catch (e) {
      print("exception happend") //: exception happend
    }
  }
 testArrayPushMaxLength()

 function testArrayLikePush(){
    print("testArrayLikePush"); //: testArrayLikePush
    let arrayLike = {
        length: 0,
        0: "initialValue"
      };
    
    try {
        //aot: [trace] aot call builtin: Function.prototype.call, caller function name:#*#testArrayLikePush@builtinArrayPush
        Array.prototype.push.call(arrayLike, 1);
        //aot: [trace] aot call builtin: Function.prototype.call, caller function name:#*#testArrayLikePush@builtinArrayPush
        Array.prototype.push.call(arrayLike, 2);
        if (arrayLike.length === 2 && arrayLike[0] === 1 && arrayLike[1] === 2) {
          print("Array-like push succeeded"); //: Array-like push succeeded
        } else {
          print("Array-like push failed");
        }
      } catch (e) {
        print("exception happened");
      }
}
testArrayLikePush()

function testArrayPushAsProto(){
    print("testArrayPushAsProto") //: testArrayPushAsProto
    //aot: [trace] aot inline builtin: Array.prototype.push, caller function name:#*#testArrayPushAsProto@builtinArrayPush
    //aot: [trace] aot inline builtin: Array.prototype.push, caller function name:#*#testArrayPushAsProto@builtinArrayPush
    var arr = new Array()
    arr.push(0)
    let temp = new Array();
    temp.__proto__ = arr;
    print(arr.push(0)) //: 2
    //aot: [trace] aot inline builtin: Array.prototype.push, caller function name:#*#testArrayPushAsProto@builtinArrayPush
    //aot: [trace] Check Type: BuiltinInstanceHClassMismatch
    print(temp.push(0)) //: 1
}
testArrayPushAsProto()

function testArrayPushNotStableArray2() {
    print("testArrayPushNotStableArray2") //: testArrayPushNotStableArray2
    var arr = [,];
    arr.__proto__.push(0);
    //aot: [trace] aot inline builtin: Array.prototype.push, caller function name:#*#testArrayPushNotStableArray2@builtinArrayPush
    //aot: [trace] Check Type: BuiltinInstanceHClassMismatch
    print(arr.push(0));  //: 2
}
testArrayPushNotStableArray2()

function testArrayPushNotStableArray3() {
    print("testArrayPushNotStableArray3") //: testArrayPushNotStableArray3
    var arr = [,];
    let k = function push() {
        return arr.push(1);
    }
    //aot: [trace] aot inline builtin: Array.prototype.push, caller function name:#*#testArrayPushNotStableArray3@builtinArrayPush
    //aot: [trace] Check Type: NotStableArray1
    arr.__proto__.push(0);
    //aot: [trace] Check Type: NotStableArray1
    print(k());  //: 2
}
testArrayPushNotStableArray3()