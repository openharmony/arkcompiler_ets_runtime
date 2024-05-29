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

let buffer = new ArrayBuffer(8);
let sample = new DataView(buffer, 0);

// NaN
sample.setFloat32(0);
print(sample.getFloat32(0))
// still NaN 
print(sample.getFloat32(0, true))
// NaN
sample.setFloat64(0)
print(sample.getFloat64())
// still NaN
print(sample.getFloat64(0))

sample.setUint8(0, 127);
sample.setUint8(1, 128);
sample.setUint8(2, 0);
sample.setUint8(3, 0);
sample.setUint8(4, 255);
sample.setUint8(5, 128);
sample.setUint8(6, 0);
sample.setUint8(7, 0);

//Infinity
print(sample.getFloat32(0))
//-Infinity
print(sample.getFloat32(4))

sample.setUint8(0, 75);
sample.setUint8(1, 75);
sample.setUint8(2, 76);
sample.setUint8(3, 76);
sample.setUint8(4, 75);
sample.setUint8(5, 75);
sample.setUint8(6, 76);
sample.setUint8(7, 76);

//32th bit is 1
sample.setInt32(0, -10000)
print(sample.getUint32(0))

//16th bit is 1
sample.setInt16(0, -100)
print(sample.getUint16(0))
//set Int NaN
sample.setUint32(0, NaN)
print(sample.getUint32(0))
print(sample.getUint32(4))

sample.setUint8(0, -1)
print(sample.getUint8(0))
print(sample.getInt8(0))

//break tagged mark
sample.setFloat64(0, 9007199254740991)
print(sample.getFloat64(0, true))
print(sample.getFloat64(0, false))

//greater safe number
sample.setFloat64(0, 9007199254740992)
print(sample.getFloat64(0, true))
print(sample.getFloat64(0, false))

sample.setFloat32(0, 0.50000001)
print(sample.getFloat32(0, true))
print(sample.getFloat32(0, false))

//special value
sample.setFloat32(0, null)
print(sample.getFloat32(0))
sample.setInt32(0, null)
print(sample.getInt32(0))
sample.setFloat64(0, true)
print(sample.getFloat64(0))