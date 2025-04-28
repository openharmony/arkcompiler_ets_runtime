/*
 * Copyright (c) 2025 Huawei Device Co., Ltd.
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

function Buffer() {}

function ReadBigInt64BE(array, offset) {
    offset = offset >>> 0;
    // not buildin typed array
    const first = array[offset];
    return BigInt(first);
}

function Test() {
    let buf = new Uint8Array([0x00, 0x00, 0x00, 0x00, 0xff, 0xff, 0xff, 0xff]);
    Object.setPrototypeOf(buf, Buffer.prototype);
    let result = ReadBigInt64BE(buf, 0);
    print(result);
}

Test();

ArkTools.jitCompileAsync(ReadBigInt64BE);
print(ArkTools.waitJitCompileFinish(ReadBigInt64BE));

Test();
