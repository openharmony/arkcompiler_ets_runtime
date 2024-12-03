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

/*
 * @tc.name:sharedbitvector
 * @tc.desc:test sharedbitvector
 * @tc.type: FUNC
 * @tc.require: issueI8QUU0
 */

// @ts-nocheck
declare function print(str: any): string;

class SuperClass {
    public obj : BitVector;
    constructor(obj : BitVector) {
        "use sendable"
        this.obj = obj;
    }
}

function newsharedclassfrom() {
    let arkPrivate = globalThis.ArkPrivate;
    var BitVector = arkPrivate.Load(arkPrivate.BitVector);
    let bitvector = new BitVector(10);
    bitvector.push(0)
    bitvector.push(1)
    
    print("Start Test newsharedclassfrom")
    try {
        let bit = new SuperClass(bitvector);
        print("bitvector pass")
    } catch (err) {
        print("bitvector fail")
    }
}

newsharedclassfrom()