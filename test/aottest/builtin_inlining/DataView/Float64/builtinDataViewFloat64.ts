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

//aot: [trace] aot inline builtin: DataView.prototype.SetFloat64, caller function name:func_main_0@builtinDataViewFloat64
sample.setFloat64(0)
print(sample.getFloat64()) //: NaN
//aot: [trace] aot inline builtin: DataView.prototype.GetFloat64, caller function name:func_main_0@builtinDataViewFloat64
print(sample.getFloat64(0)) //: NaN
print(sample.getBigInt64(0).toString(16)) //: 7ff8000000000000

//aot: [trace] aot inline builtin: DataView.prototype.SetFloat64, caller function name:func_main_0@builtinDataViewFloat64
sample.setFloat64(0, NaN, true)
//aot: [trace] aot inline builtin: DataView.prototype.GetFloat64, caller function name:func_main_0@builtinDataViewFloat64
print(sample.getFloat64(0, true)) //: NaN
print(sample.getBigInt64(0, true).toString(16)) //: 7ff8000000000000

sample.setBigUint64 (0, 0xf87fn);
//aot: [trace] aot inline builtin: DataView.prototype.GetFloat64, caller function name:func_main_0@builtinDataViewFloat64
print(sample.getFloat64(0))     //: 3.143e-319
print(sample.getBigInt64(0).toString(16))     //: f87f

sample.setBigUint64 (0, 0xf87fn, true);
//aot: [trace] aot inline builtin: DataView.prototype.GetFloat64, caller function name:func_main_0@builtinDataViewFloat64
print(sample.getFloat64(0, true))     //: 3.143e-319
print(sample.getBigInt64(0, true).toString(16))     //: f87f

//aot: [trace] aot inline builtin: DataView.prototype.SetUint8, caller function name:func_main_0@builtinDataViewFloat64
sample.setUint8(0, 0x7f);
//aot: [trace] aot inline builtin: DataView.prototype.SetUint8, caller function name:func_main_0@builtinDataViewFloat64
sample.setUint8(1, 0xf8);
//aot: [trace] aot inline builtin: DataView.prototype.SetUint8, caller function name:func_main_0@builtinDataViewFloat64
sample.setUint8(2, 0x00);
//aot: [trace] aot inline builtin: DataView.prototype.SetUint8, caller function name:func_main_0@builtinDataViewFloat64
sample.setUint8(3, 0x00);
//aot: [trace] aot inline builtin: DataView.prototype.SetUint8, caller function name:func_main_0@builtinDataViewFloat64
sample.setUint8(4, 0x00);
//aot: [trace] aot inline builtin: DataView.prototype.SetUint8, caller function name:func_main_0@builtinDataViewFloat64
sample.setUint8(5, 0x00);
//aot: [trace] aot inline builtin: DataView.prototype.SetUint8, caller function name:func_main_0@builtinDataViewFloat64
sample.setUint8(6, 0x00);
//aot: [trace] aot inline builtin: DataView.prototype.SetUint8, caller function name:func_main_0@builtinDataViewFloat64
sample.setUint8(7, 0x00);
//aot: [trace] aot inline builtin: DataView.prototype.GetFloat64, caller function name:func_main_0@builtinDataViewFloat64
print(sample.getFloat64(0))      //: NaN
print(sample.getBigInt64(0).toString(16))        //: 7ff8000000000000

//aot: [trace] aot inline builtin: DataView.prototype.SetUint8, caller function name:func_main_0@builtinDataViewFloat64
sample.setUint8(0, 0x7f);
//aot: [trace] aot inline builtin: DataView.prototype.SetUint8, caller function name:func_main_0@builtinDataViewFloat64
sample.setUint8(1, 0xf8);
//aot: [trace] aot inline builtin: DataView.prototype.SetUint8, caller function name:func_main_0@builtinDataViewFloat64
sample.setUint8(2, 0x00);
//aot: [trace] aot inline builtin: DataView.prototype.SetUint8, caller function name:func_main_0@builtinDataViewFloat64
sample.setUint8(3, 0x00);
//aot: [trace] aot inline builtin: DataView.prototype.SetUint8, caller function name:func_main_0@builtinDataViewFloat64
sample.setUint8(4, 0x00);
//aot: [trace] aot inline builtin: DataView.prototype.SetUint8, caller function name:func_main_0@builtinDataViewFloat64
sample.setUint8(5, 0x00);
//aot: [trace] aot inline builtin: DataView.prototype.SetUint8, caller function name:func_main_0@builtinDataViewFloat64
sample.setUint8(6, 0x00);
//aot: [trace] aot inline builtin: DataView.prototype.SetUint8, caller function name:func_main_0@builtinDataViewFloat64
sample.setUint8(7, 0x00);
//aot: [trace] aot inline builtin: DataView.prototype.GetFloat64, caller function name:func_main_0@builtinDataViewFloat64
print(sample.getFloat64(0, true))        //: 3.143e-319
print(sample.getBigInt64(0, true).toString(16))      //: f87f

//aot: [trace] aot inline builtin: DataView.prototype.SetUint8, caller function name:func_main_0@builtinDataViewFloat64
sample.setUint8(0, 0x00);
//aot: [trace] aot inline builtin: DataView.prototype.SetUint8, caller function name:func_main_0@builtinDataViewFloat64
sample.setUint8(1, 0x00);
//aot: [trace] aot inline builtin: DataView.prototype.SetUint8, caller function name:func_main_0@builtinDataViewFloat64
sample.setUint8(2, 0x00);
//aot: [trace] aot inline builtin: DataView.prototype.SetUint8, caller function name:func_main_0@builtinDataViewFloat64
sample.setUint8(3, 0x00);
//aot: [trace] aot inline builtin: DataView.prototype.SetUint8, caller function name:func_main_0@builtinDataViewFloat64
sample.setUint8(4, 0x00);
//aot: [trace] aot inline builtin: DataView.prototype.SetUint8, caller function name:func_main_0@builtinDataViewFloat64
sample.setUint8(5, 0x00);
//aot: [trace] aot inline builtin: DataView.prototype.SetUint8, caller function name:func_main_0@builtinDataViewFloat64
sample.setUint8(6, 0xf8);
//aot: [trace] aot inline builtin: DataView.prototype.SetUint8, caller function name:func_main_0@builtinDataViewFloat64
sample.setUint8(7, 0x7f);
//aot: [trace] aot inline builtin: DataView.prototype.GetFloat64, caller function name:func_main_0@builtinDataViewFloat64
print(sample.getFloat64(0))      //: 3.143e-319
print(sample.getBigInt64(0).toString(16))        //: f87f

//aot: [trace] aot inline builtin: DataView.prototype.SetUint8, caller function name:func_main_0@builtinDataViewFloat64
sample.setUint8(0, 0x00);
//aot: [trace] aot inline builtin: DataView.prototype.SetUint8, caller function name:func_main_0@builtinDataViewFloat64
sample.setUint8(1, 0x00);
//aot: [trace] aot inline builtin: DataView.prototype.SetUint8, caller function name:func_main_0@builtinDataViewFloat64
sample.setUint8(2, 0x00);
//aot: [trace] aot inline builtin: DataView.prototype.SetUint8, caller function name:func_main_0@builtinDataViewFloat64
sample.setUint8(3, 0x00);
//aot: [trace] aot inline builtin: DataView.prototype.SetUint8, caller function name:func_main_0@builtinDataViewFloat64
sample.setUint8(4, 0x00);
//aot: [trace] aot inline builtin: DataView.prototype.SetUint8, caller function name:func_main_0@builtinDataViewFloat64
sample.setUint8(5, 0x00);
//aot: [trace] aot inline builtin: DataView.prototype.SetUint8, caller function name:func_main_0@builtinDataViewFloat64
sample.setUint8(6, 0xf8);
//aot: [trace] aot inline builtin: DataView.prototype.SetUint8, caller function name:func_main_0@builtinDataViewFloat64
sample.setUint8(7, 0x7f);
//aot: [trace] aot inline builtin: DataView.prototype.GetFloat64, caller function name:func_main_0@builtinDataViewFloat64
print(sample.getFloat64(0, true))        //: NaN
print(sample.getBigInt64(0, true).toString(16))      //: 7ff8000000000000

// Test case coverage for crash scenarios
let buff = new ArrayBuffer(8);
let u64 = new BigUint64Array(buff);
let dv = new DataView(buff);
u64[0] = 0xfff78715227b5c40n;
//aot: [trace] aot inline builtin: DataView.prototype.GetFloat64, caller function name:func_main_0@builtinDataViewFloat64
let res = dv.getFloat64(0, false);
//aot: [trace] aot inline builtin: Math.abs, caller function name:func_main_0@builtinDataViewFloat64
print(Math.abs(res - 0));   //: 113.92395532873341