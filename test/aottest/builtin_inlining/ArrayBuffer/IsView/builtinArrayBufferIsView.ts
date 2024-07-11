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
declare function print(arg:any):string;
declare interface ArkTools {
    isAOTCompiled(args: any): boolean;
}

let arraybuffer = new ArrayBuffer(10)
let dataView = new DataView(arraybuffer)
print(ArrayBuffer.isView(dataView)) //: true
print(ArrayBuffer.isView(dataView, 0)) //: true
let str = "123"
print(ArrayBuffer.isView(str)) //: false
print(ArrayBuffer.isView()) //: false
print(ArrayBuffer.isView(123)) //: false
print(ArrayBuffer.isView(new Float32Array())) //: true
print(ArrayBuffer.isView({})) //: false
print(ArrayBuffer.isView([])) //: false
print(ArrayBuffer.isView(undefined)) //: false

// Check inside try-block
try {
  print('try') //: try
  print(ArrayBuffer.isView({})) //: false
} catch(e) { 
}

let obj = {};
obj.valueOf = (() => { return 5; })
print(ArrayBuffer.isView(obj)); //: false

function Throwing() {
    this.value = 2;
    Throwing.prototype.valueOf = function() {
        if (this.value > 0) {
            throw new Error("positive");
        }
        return this.value;
    }
}
let throwingObj = new Throwing();
try {
    print(ArrayBuffer.isView(throwingObj)); //: false
} catch(e) {
    print(e);
} finally {
    print(ArrayBuffer.isView(obj)); //: false
}

// Replace standard builtin
function replace(a : any) {
  return a;
}

let true_isView = ArrayBuffer.isView
ArrayBuffer.isView = replace

print(ArrayBuffer.isView(undefined)); //: undefined
ArrayBuffer.isView = true_isView
print(ArrayBuffer.isView(undefined)); //: false


function doIsView(x: any): any {
  return ArrayBuffer.isView(x);
}

function printIsView(x: any) {
  try {
      print(doIsView(x));
  } finally {
  }
}

if (ArkTools.isAOTCompiled(printIsView)) {
  // Replace standard builtin after call to standard builtin was profiled
  ArrayBuffer.isView = replace
}
printIsView(-1); //pgo: false
//aot: [trace] Check Type: NotCallTarget1
//aot: -1

printIsView("abc"); //pgo: false
//aot: [trace] Check Type: NotCallTarget1
//aot: abc

ArrayBuffer.isView = true_isView