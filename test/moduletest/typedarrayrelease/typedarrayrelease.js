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

/*
 * @tc.name:typedarray
 * @tc.desc:test typedarray
 * @tc.type: FUNC
 * @tc.require: issueIBFBG7
 */

// This case aims to check whether proto modifying causes out-of-bounds write of the newly generated typed array.
{
    let uint32Array = new Uint32Array(new ArrayBuffer(0x100));
    let uint8Array = new Uint8Array([21, 31]);
    uint32Array.__proto__ = uint8Array;
    for(let i = 0; i < 100; i++)
    {
        uint32Array.slice(-60);
        uint32Array.with(1, 20);
        uint32Array.toSorted();
    }
    print("Test runs successfully!");
}

// This case aims to check while proto was modified, type conversion of the newly generated typed array works normally.
{
    let uint32Array = new Uint32Array(new ArrayBuffer(0x100));
    uint32Array[62] = 0x8fffffff;
    uint32Array[63] = 0x8fffffff;
    let float32Array = new Float32Array([12.3333, 1.777]);
    uint32Array.__proto__ = float32Array;
    print(uint32Array.slice(-2));
}
