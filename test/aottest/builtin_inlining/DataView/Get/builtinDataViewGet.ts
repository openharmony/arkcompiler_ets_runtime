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
declare function print(arg: any): string;
declare interface ArkTools {
    isAOTCompiled(args: any): boolean;
}
let buffer = new ArrayBuffer(8);
let sample = new DataView(buffer, 0);

//aot: [trace] aot inline builtin: DataView.prototype.SetFloat32, caller function name:func_main_0@builtinDataViewGet
sample.setFloat32(0);
//aot: [trace] aot inline builtin: DataView.prototype.GetFloat32, caller function name:func_main_0@builtinDataViewGet
print(sample.getFloat32(0)) //: NaN
//aot: [trace] aot inline builtin: DataView.prototype.GetFloat32, caller function name:func_main_0@builtinDataViewGet
print(sample.getFloat32(0, true)) //: NaN
//aot: [trace] aot inline builtin: DataView.prototype.SetFloat64, caller function name:func_main_0@builtinDataViewGet
sample.setFloat64(0)
print(sample.getFloat64()) //: NaN
//aot: [trace] aot inline builtin: DataView.prototype.GetFloat64, caller function name:func_main_0@builtinDataViewGet
print(sample.getFloat64(0)) //: NaN

//aot: [trace] aot inline builtin: DataView.prototype.SetUint8, caller function name:func_main_0@builtinDataViewGet
sample.setUint8(0, 127);
//aot: [trace] aot inline builtin: DataView.prototype.SetUint8, caller function name:func_main_0@builtinDataViewGet
sample.setUint8(1, 128);
//aot: [trace] aot inline builtin: DataView.prototype.SetUint8, caller function name:func_main_0@builtinDataViewGet
sample.setUint8(2, 0);
//aot: [trace] aot inline builtin: DataView.prototype.SetUint8, caller function name:func_main_0@builtinDataViewGet
sample.setUint8(3, 0);
//aot: [trace] aot inline builtin: DataView.prototype.SetUint8, caller function name:func_main_0@builtinDataViewGet
sample.setUint8(4, 255);
//aot: [trace] aot inline builtin: DataView.prototype.SetUint8, caller function name:func_main_0@builtinDataViewGet
sample.setUint8(5, 128);
//aot: [trace] aot inline builtin: DataView.prototype.SetUint8, caller function name:func_main_0@builtinDataViewGet
sample.setUint8(6, 0);
//aot: [trace] aot inline builtin: DataView.prototype.SetUint8, caller function name:func_main_0@builtinDataViewGet
sample.setUint8(7, 0);

//aot: [trace] aot inline builtin: DataView.prototype.GetFloat32, caller function name:func_main_0@builtinDataViewGet
print(sample.getFloat32(0)) //: Infinity
//aot: [trace] aot inline builtin: DataView.prototype.GetFloat32, caller function name:func_main_0@builtinDataViewGet
print(sample.getFloat32(4)) //: -Infinity

//aot: [trace] aot inline builtin: DataView.prototype.SetUint8, caller function name:func_main_0@builtinDataViewGet
sample.setUint8(0, 75);
//aot: [trace] aot inline builtin: DataView.prototype.SetUint8, caller function name:func_main_0@builtinDataViewGet
sample.setUint8(1, 75);
//aot: [trace] aot inline builtin: DataView.prototype.SetUint8, caller function name:func_main_0@builtinDataViewGet
sample.setUint8(2, 76);
//aot: [trace] aot inline builtin: DataView.prototype.SetUint8, caller function name:func_main_0@builtinDataViewGet
sample.setUint8(3, 76);
//aot: [trace] aot inline builtin: DataView.prototype.SetUint8, caller function name:func_main_0@builtinDataViewGet
sample.setUint8(4, 75);
//aot: [trace] aot inline builtin: DataView.prototype.SetUint8, caller function name:func_main_0@builtinDataViewGet
sample.setUint8(5, 75);
//aot: [trace] aot inline builtin: DataView.prototype.SetUint8, caller function name:func_main_0@builtinDataViewGet
sample.setUint8(6, 76);
//aot: [trace] aot inline builtin: DataView.prototype.SetUint8, caller function name:func_main_0@builtinDataViewGet
sample.setUint8(7, 76);

//32th bit is 1
//aot: [trace] aot inline builtin: DataView.prototype.SetInt32, caller function name:func_main_0@builtinDataViewGet
sample.setInt32(0, -10000)
//aot: [trace] aot inline builtin: DataView.prototype.GetUint32, caller function name:func_main_0@builtinDataViewGet
print(sample.getUint32(0)) //: 4294957296

//16th bit is 1
//aot: [trace] aot inline builtin: DataView.prototype.SetInt16, caller function name:func_main_0@builtinDataViewGet
sample.setInt16(0, -100)
//aot: [trace] aot inline builtin: DataView.prototype.GetUint16, caller function name:func_main_0@builtinDataViewGet
print(sample.getUint16(0)) //: 65436
//set Int NaN
//aot: [trace] aot inline builtin: DataView.prototype.SetUint32, caller function name:func_main_0@builtinDataViewGet
sample.setUint32(0, NaN)
//aot: [trace] aot inline builtin: DataView.prototype.GetUint32, caller function name:func_main_0@builtinDataViewGet
print(sample.getUint32(0)) //: 0
//aot: [trace] aot inline builtin: DataView.prototype.GetUint32, caller function name:func_main_0@builtinDataViewGet
print(sample.getUint32(4)) //: 1263225932

//aot: [trace] aot inline builtin: DataView.prototype.SetUint8, caller function name:func_main_0@builtinDataViewGet
print(sample.setUint8(0, -1)) //: undefined
//aot: [trace] aot inline builtin: DataView.prototype.GetUint8, caller function name:func_main_0@builtinDataViewGet
print(sample.getUint8(0)) //: 255
//aot: [trace] aot inline builtin: DataView.prototype.GetInt8, caller function name:func_main_0@builtinDataViewGet
print(sample.getInt8(0)) //: -1

print(sample.setUint8(0, -1, 90, 78)) //: undefined
//aot: [trace] aot inline builtin: DataView.prototype.GetUint8, caller function name:func_main_0@builtinDataViewGet
print(sample.getUint8(0)) //: 255
//aot: [trace] aot inline builtin: DataView.prototype.GetInt8, caller function name:func_main_0@builtinDataViewGet
print(sample.getInt8(0)) //: -1

//break tagged mark
//aot: [trace] aot inline builtin: DataView.prototype.SetFloat64, caller function name:func_main_0@builtinDataViewGet
sample.setFloat64(0, 9007199254740991)
//aot: [trace] aot inline builtin: DataView.prototype.GetFloat64, caller function name:func_main_0@builtinDataViewGet
print(sample.getFloat64(0, true)) //: NaN
//aot: [trace] aot inline builtin: DataView.prototype.GetFloat64, caller function name:func_main_0@builtinDataViewGet
print(sample.getFloat64(0, false)) //: 9007199254740991

//greater safe number
//aot: [trace] aot inline builtin: DataView.prototype.SetFloat64, caller function name:func_main_0@builtinDataViewGet
sample.setFloat64(0, 9007199254740992)
//aot: [trace] aot inline builtin: DataView.prototype.GetFloat64, caller function name:func_main_0@builtinDataViewGet
print(sample.getFloat64(0, true)) //: 8.128e-320
//aot: [trace] aot inline builtin: DataView.prototype.GetFloat64, caller function name:func_main_0@builtinDataViewGet
print(sample.getFloat64(0, false)) //: 9007199254740992

//aot: [trace] aot inline builtin: DataView.prototype.SetFloat32, caller function name:func_main_0@builtinDataViewGet
sample.setFloat32(0, 0.50000001)
//aot: [trace] aot inline builtin: DataView.prototype.GetFloat32, caller function name:func_main_0@builtinDataViewGet
print(sample.getFloat32(0, true)) //: 8.828180325246348e-44
//aot: [trace] aot inline builtin: DataView.prototype.GetFloat32, caller function name:func_main_0@builtinDataViewGet
print(sample.getFloat32(0, false)) //: 0.5

//special value
//aot: [trace] aot inline builtin: DataView.prototype.SetFloat32, caller function name:func_main_0@builtinDataViewGet
sample.setFloat32(0, null)
//aot: [trace] aot inline builtin: DataView.prototype.GetFloat32, caller function name:func_main_0@builtinDataViewGet
print(sample.getFloat32(0)) //: 0
//aot: [trace] aot inline builtin: DataView.prototype.SetInt32, caller function name:func_main_0@builtinDataViewGet
sample.setInt32(0, null)
//aot: [trace] aot inline builtin: DataView.prototype.GetInt32, caller function name:func_main_0@builtinDataViewGet
print(sample.getInt32(0)) //: 0
//aot: [trace] aot inline builtin: DataView.prototype.SetFloat64, caller function name:func_main_0@builtinDataViewGet
sample.setFloat64(0, true)
//aot: [trace] aot inline builtin: DataView.prototype.GetFloat64, caller function name:func_main_0@builtinDataViewGet
print(sample.getFloat64(0)) //: 1
//aot: [trace] Check Type: NotBool1
print(sample.getFloat64(0, 'abc')) //: 3.03865e-319
print(sample.getFloat64('abc')) //: 1

// Check inside try-block
try {
    print(sample.setUint8(0, -1, 90, 78)) //: undefined
    print(sample.getUint8(0)) //: 255
} catch(e) {
}

let obj = {};
obj.valueOf = (() => { return 5; })
print(sample.setUint8(obj)); //: undefined

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
    print(sample.setUint8(throwingObj));
} catch(e) {
    print(e); //: Error: positive
} finally {
    print(sample.setUint8(obj)); //: undefined
}

sample.getUint8(0)
sample.setInt16(0, -100)

// Replace standard builtin
function replace(a : any) {
  return a;
}

let true_get = sample.getInt8
sample.getInt8 = replace

print(sample.getInt8(0)); //: 0
sample.getInt8 = true_get
print(sample.getInt8(0)); //: -1


function doGet(x: any): any {
  return sample.getInt8(x);
}

function printGet(x: any) {
  try {
      print(doGet(x));
  } finally {
  }
}

if (ArkTools.isAOTCompiled(printGet)) {
  // Replace standard builtin after call to standard builtin was profiled
  sample.getInt8 = replace
}
printGet(1); //pgo: -100
//aot: [trace] Check Type: NotCallTarget1
//aot: 1

printGet("abc"); //pgo: -1
//aot: [trace] Check Type: NotCallTarget1
//aot: abc

sample.getInt8 = true_get

function getCase1() {
    print('case 1 get') //: case 1 get
    let buffer = new ArrayBuffer(8);
    let sample1 = new DataView(buffer, 0);
    let sample2 = new DataView(buffer, 0);
    sample2.garbage = function(x: any): any {
        return undefined;
    }

    //aot: [trace] aot inline builtin: DataView.prototype.SetInt32, caller function name:#*#getCase1@builtinDataViewGet
    sample1.setInt32(0, null)
    //aot: [trace] aot inline builtin: DataView.prototype.SetInt32, caller function name:#*#getCase1@builtinDataViewGet
    sample2.setInt32(0, null)

    //aot: [trace] aot inline builtin: DataView.prototype.GetInt32, caller function name:#*#getCase1@builtinDataViewGet
    print(sample1.getInt32(0)); //: 0
    //aot: [trace] aot inline builtin: DataView.prototype.GetInt32, caller function name:#*#getCase1@builtinDataViewGet
    print(sample2.getInt32(0)); //: 0
}
getCase1()


function getCase2() {
    print('case 2 get') //: case 2 get
    let buffer = new ArrayBuffer(8);
    let sample1 = new DataView(buffer, 0);
    let sample2 = new DataView(buffer, 0);
    sample2.getInt32 = function(x: any) {
        return x + 1
    }

    //aot: [trace] aot inline builtin: Object.getPrototypeOf, caller function name:#*#getCase2@builtinDataViewGet
    print(Object.getPrototypeOf(sample2) === DataView.prototype) //: true

    //aot: [trace] aot inline builtin: DataView.prototype.SetInt32, caller function name:#*#getCase2@builtinDataViewGet
    sample1.setInt32(0, null)
    //aot: [trace] aot inline builtin: DataView.prototype.SetInt32, caller function name:#*#getCase2@builtinDataViewGet
    sample2.setInt32(0, null)

    //aot: [trace] aot inline builtin: DataView.prototype.GetInt32, caller function name:#*#getCase2@builtinDataViewGet
    print(sample1.getInt32(0)); //: 0
    //aot: [trace] aot inline function name: #*@2*#@builtinDataViewGet caller function name: #*#getCase2@builtinDataViewGet
    print(sample2.getInt32(0)); //: 1
}
getCase2()


function getCase3() {
    print('case 3 get') //: case 3 get
    let buffer = new ArrayBuffer(8);
    let msample = new DataView(buffer, 0);
    let true_getInt32 = msample.getInt32
    let mimicDataView = {
        getInt32: true_getInt32,
    }

    //aot: [trace] aot inline builtin: DataView.prototype.SetInt32, caller function name:#*#getCase3@builtinDataViewGet
    msample.setInt32(0, null)
    //aot: [trace] aot inline builtin: DataView.prototype.GetInt32, caller function name:#*#getCase3@builtinDataViewGet
    print(msample.getInt32(0)); //: 0
    //aot: [trace] aot call builtin: Object.SetPrototypeOf, caller function name:#*#getCase3@builtinDataViewGet
    Object.setPrototypeOf(msample, mimicDataView)

    //aot: [trace] aot inline builtin: DataView.prototype.GetInt32, caller function name:#*#getCase3@builtinDataViewGet
    print(msample.getInt32(0)); //: 0
}
getCase3()


function getCase4() {
    print('case 4 get') //: case 4 get
    let buffer = new ArrayBuffer(8);
    let sample1 = new DataView(buffer, 0);
    let sample2 = new DataView(buffer, 0);
    let true_setInt32 = sample2.setInt32
    let notDataView = {
        getInt32(x: any) {
            return -100
        },
        setInt32: true_setInt32
    }
    //aot: [trace] aot call builtin: Object.SetPrototypeOf, caller function name:#*#getCase4@builtinDataViewGet
    Object.setPrototypeOf(sample2, notDataView)
    //aot: [trace] aot inline builtin: DataView.prototype.SetInt32, caller function name:#*#getCase4@builtinDataViewGet
    sample1.setInt32(0, null)
    //aot: [trace] aot inline builtin: DataView.prototype.SetInt32, caller function name:#*#getCase4@builtinDataViewGet
    sample2.setInt32(0, null)

    //aot: [trace] aot inline builtin: DataView.prototype.GetInt32, caller function name:#*#getCase4@builtinDataViewGet
    print(sample1.getInt32(0)); //: 0
    //aot: [trace] aot inline function name: #*@3*#getInt32@builtinDataViewGet caller function name: #*#getCase4@builtinDataViewGet
    print(sample2.getInt32(0)); //: -100
}
getCase4()


function getCase5() {
    print('case 5 get') //: case 5 get
    let buffer = new ArrayBuffer(8);
    let sample1 = new DataView(buffer, 0);
    DataView.prototype.getInt32 = function(x: any) {
        return x + 45
    }

    //aot: [trace] aot inline function name: #*@4*#@builtinDataViewGet caller function name: #*#getCase5@builtinDataViewGet
    print(sample1.getInt32(1)); //: 46
}
getCase5()
