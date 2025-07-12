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
let buffer = new ArrayBuffer(4);
let sample = new DataView(buffer, 0);

//aot: [trace] aot inline builtin: DataView.prototype.SetFloat32, caller function name:func_main_0@builtinDataViewFloat32
sample.setFloat32(0);
//aot: [trace] aot inline builtin: DataView.prototype.GetFloat32, caller function name:func_main_0@builtinDataViewFloat32
print(sample.getFloat32(0)) //: NaN
//aot: [trace] aot inline builtin: DataView.prototype.GetUint32, caller function name:func_main_0@builtinDataViewFloat32
//aot: [trace] aot call builtin: Number.ToStringFunc, caller function name:func_main_0@builtinDataViewFloat32
print(sample.getUint32(0).toString(16)) //: 7fc00000

//aot: [trace] aot inline builtin: DataView.prototype.SetFloat32, caller function name:func_main_0@builtinDataViewFloat32
sample.setFloat32(0, NaN, true);
//aot: [trace] aot inline builtin: DataView.prototype.GetFloat32, caller function name:func_main_0@builtinDataViewFloat32
print(sample.getFloat32(0, true)) //: NaN
//aot: [trace] aot inline builtin: DataView.prototype.GetUint32, caller function name:func_main_0@builtinDataViewFloat32
//aot: [trace] aot call builtin: Number.ToStringFunc, caller function name:func_main_0@builtinDataViewFloat32
print(sample.getUint32(0, true).toString(16)) //: 7fc00000

//aot: [trace] aot inline builtin: DataView.prototype.SetUint32, caller function name:func_main_0@builtinDataViewFloat32
sample.setUint32(0, 0xC07F);
//aot: [trace] aot inline builtin: DataView.prototype.GetFloat32, caller function name:func_main_0@builtinDataViewFloat32
print(sample.getFloat32(0)) //: 6.905458702346266e-41
//aot: [trace] aot inline builtin: DataView.prototype.GetUint32, caller function name:func_main_0@builtinDataViewFloat32
//aot: [trace] aot call builtin: Number.ToStringFunc, caller function name:func_main_0@builtinDataViewFloat32
print(sample.getUint32(0).toString(16)) //: c07f

//aot: [trace] aot inline builtin: DataView.prototype.SetUint32, caller function name:func_main_0@builtinDataViewFloat32
sample.setUint32(0, 0xC07F, true);
//aot: [trace] aot inline builtin: DataView.prototype.GetFloat32, caller function name:func_main_0@builtinDataViewFloat32
print(sample.getFloat32(0, true)) //: 6.905458702346266e-41
//aot: [trace] aot inline builtin: DataView.prototype.GetUint32, caller function name:func_main_0@builtinDataViewFloat32
//aot: [trace] aot call builtin: Number.ToStringFunc, caller function name:func_main_0@builtinDataViewFloat32
print(sample.getUint32(0, true).toString(16)) //: c07f

//aot: [trace] aot inline builtin: DataView.prototype.SetUint8, caller function name:func_main_0@builtinDataViewFloat32
sample.setUint8(0, 0x00);
//aot: [trace] aot inline builtin: DataView.prototype.SetUint8, caller function name:func_main_0@builtinDataViewFloat32
sample.setUint8(1, 0x00);
//aot: [trace] aot inline builtin: DataView.prototype.SetUint8, caller function name:func_main_0@builtinDataViewFloat32
sample.setUint8(2, 0xC0);
//aot: [trace] aot inline builtin: DataView.prototype.SetUint8, caller function name:func_main_0@builtinDataViewFloat32
sample.setUint8(3, 0x7F);
//aot: [trace] aot inline builtin: DataView.prototype.GetFloat32, caller function name:func_main_0@builtinDataViewFloat32
print(sample.getFloat32(0)) //: 6.905458702346266e-41
//aot: [trace] aot inline builtin: DataView.prototype.GetUint32, caller function name:func_main_0@builtinDataViewFloat32
//aot: [trace] aot call builtin: Number.ToStringFunc, caller function name:func_main_0@builtinDataViewFloat32
print(sample.getUint32(0).toString(16)) //: c07f

//aot: [trace] aot inline builtin: DataView.prototype.SetUint8, caller function name:func_main_0@builtinDataViewFloat32
sample.setUint8(0, 0x00);
//aot: [trace] aot inline builtin: DataView.prototype.SetUint8, caller function name:func_main_0@builtinDataViewFloat32
sample.setUint8(1, 0x00);
//aot: [trace] aot inline builtin: DataView.prototype.SetUint8, caller function name:func_main_0@builtinDataViewFloat32
sample.setUint8(2, 0xC0);
//aot: [trace] aot inline builtin: DataView.prototype.SetUint8, caller function name:func_main_0@builtinDataViewFloat32
sample.setUint8(3, 0x7F);
//aot: [trace] aot inline builtin: DataView.prototype.GetFloat32, caller function name:func_main_0@builtinDataViewFloat32
print(sample.getFloat32(0, true)) //: NaN
//aot: [trace] aot inline builtin: DataView.prototype.GetUint32, caller function name:func_main_0@builtinDataViewFloat32
//aot: [trace] aot call builtin: Number.ToStringFunc, caller function name:func_main_0@builtinDataViewFloat32
print(sample.getUint32(0, true).toString(16)) //: 7fc00000

//aot: [trace] aot inline builtin: DataView.prototype.SetUint8, caller function name:func_main_0@builtinDataViewFloat32
sample.setUint8(0, 0x7F);
//aot: [trace] aot inline builtin: DataView.prototype.SetUint8, caller function name:func_main_0@builtinDataViewFloat32
sample.setUint8(1, 0xC0);
//aot: [trace] aot inline builtin: DataView.prototype.SetUint8, caller function name:func_main_0@builtinDataViewFloat32
sample.setUint8(2, 0x00);
//aot: [trace] aot inline builtin: DataView.prototype.SetUint8, caller function name:func_main_0@builtinDataViewFloat32
sample.setUint8(3, 0x00);
//aot: [trace] aot inline builtin: DataView.prototype.GetFloat32, caller function name:func_main_0@builtinDataViewFloat32
print(sample.getFloat32(0)) //: NaN
//aot: [trace] aot inline builtin: DataView.prototype.GetUint32, caller function name:func_main_0@builtinDataViewFloat32
//aot: [trace] aot call builtin: Number.ToStringFunc, caller function name:func_main_0@builtinDataViewFloat32
print(sample.getUint32(0).toString(16)) //: 7fc00000

//aot: [trace] aot inline builtin: DataView.prototype.SetUint8, caller function name:func_main_0@builtinDataViewFloat32
sample.setUint8(0, 0x7F);
//aot: [trace] aot inline builtin: DataView.prototype.SetUint8, caller function name:func_main_0@builtinDataViewFloat32
sample.setUint8(1, 0xC0);
//aot: [trace] aot inline builtin: DataView.prototype.SetUint8, caller function name:func_main_0@builtinDataViewFloat32
sample.setUint8(2, 0x00);
//aot: [trace] aot inline builtin: DataView.prototype.SetUint8, caller function name:func_main_0@builtinDataViewFloat32
sample.setUint8(3, 0x00);
//aot: [trace] aot inline builtin: DataView.prototype.GetFloat32, caller function name:func_main_0@builtinDataViewFloat32
print(sample.getFloat32(0, true)) //: 6.905458702346266e-41
//aot: [trace] aot inline builtin: DataView.prototype.GetUint32, caller function name:func_main_0@builtinDataViewFloat32
//aot: [trace] aot call builtin: Number.ToStringFunc, caller function name:func_main_0@builtinDataViewFloat32
print(sample.getUint32(0, true).toString(16)) //: c07f