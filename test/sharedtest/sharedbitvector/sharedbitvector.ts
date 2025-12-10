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

function TestBitVector() {
    let arkPrivate = globalThis.ArkPrivate;
    var BitVector = arkPrivate.Load(arkPrivate.BitVector);
    let bitvector = new BitVector(10);

    try {
        print("TestBitVector ",[0])
        print("TestBitVector ",[1])
        print("TestBitVector ",[2])
        print("TestBitVector ",[3])
        print("TestBitVector ",[4])
        print("TestBitVector ",[5])
        print("TestBitVector ",[6])
        print("TestBitVector ",[7])
        print("TestBitVector ",[8])
        print("TestBitVector ",[9])

    } catch (error) {
        print("TestBitVector failed. code: " + error.code);
    }    

    try {
        print("TestBitVector ",[10])
    } catch (error) {
        print("TestBitVector failed. code: " + error.code);
    }

    try {
        print("TestBitVector ",[-1])
    } catch (error) {
        print("TestBitVector failed. code: " + error.code);
    }

    try {
        print("TestBitVector ",[-10])
    } catch (error) {
        print("TestBitVector failed. code: " + error.code);
    }

    try {
        print("TestBitVector ",['1'])
    } catch (error) {
        print("TestBitVector failed. code: " + error.code);
    }

    try {
        print("TestBitVector ",['-10'])
    } catch (error) {
        print("TestBitVector failed. code: " + error.code);
    }
}

function TestBitVectorPop() {
    let arkPrivate = globalThis.ArkPrivate;
    var BitVector = arkPrivate.Load(arkPrivate.BitVector);
    let bitvector = new BitVector(10);
    bitvector.push(0)
    bitvector.push(1)

    try {
        print("TestBitVectorPop pop ", bitvector.pop())
        print("TestBitVectorPop length ", bitvector.length)
    } catch (error) {
        print("TestBitVectorPop failed. code: " + error.code);
    }
}

function TestBitVectorHas() {
    let arkPrivate = globalThis.ArkPrivate;
    var BitVector = arkPrivate.Load(arkPrivate.BitVector);
    let bitvector = new BitVector(10);
    bitvector.push(0)
    bitvector.push(1)
    bitvector.push(0)
    bitvector.push(1)
    bitvector.push(0)
    bitvector.push(0)
    bitvector.push(0)
    bitvector.push(1)
    bitvector.push(1)
    bitvector.push(1)
    print("TestBitVectorHas has ",bitvector.has(0, 1, 5))
    print("TestBitVectorHas has ",bitvector.has(1, 0, 9))

    try {
        print("TestBitVectorHas has ",bitvector.has(1, 1, 100))
    } catch (error) {
        print("TestBitVectorHas has failed. code: " + error.code);
    }

    try {
        print("TestBitVectorHas has ",bitvector.has(1, -1, 100))
    } catch (error) {
        print("TestBitVectorHas has failed. code: " + error.code);
    }

    try {
        print("TestBitVectorPop has ",bitvector.has(1, 8, 1))
    } catch (error) {
        print("TestBitVectorHas has failed. code: " + error.code);
    }

    try {
        print("TestBitVectorPop has ",bitvector.has(0, -100, 0))
    } catch (error) {
        print("TestBitVectorHas has failed. code: " + error.code);
    }

    try {
        print("TestBitVectorPop has ",bitvector.has(0, 0, -10))
    } catch (error) {
        print("TestBitVectorHas has failed. code: " + error.code);
    }

    try {
        print("TestBitVectorPop has ",bitvector.has('0', 0, 10))
    } catch (error) {
        print("TestBitVectorHas has failed. code: " + error.code);
    }

    try {
        print("TestBitVectorPop has ",bitvector.has('1', 0, 10))
    } catch (error) {
        print("TestBitVectorHas has failed. code: " + error.code);
    }
}

function TestBitVectorSetBitsByRange() {
    let arkPrivate = globalThis.ArkPrivate;
    var BitVector = arkPrivate.Load(arkPrivate.BitVector);
    let bitvector = new BitVector(10);
    bitvector.push(0)
    bitvector.push(1)
    bitvector.push(0)
    bitvector.push(1)
    bitvector.push(0)
    bitvector.push(0)
    bitvector.push(0)
    bitvector.push(1)
    bitvector.push(1)
    bitvector.push(1)
    print("TestBitVectorSetBitsByRange ", bitvector[0])
    print("TestBitVectorSetBitsByRange ", bitvector[1])

    try {
        bitvector.setBitsByRange(1, 5, 10);
        print("TestBitVectorSetBitsByRange ", bitvector[5])
        print("TestBitVectorSetBitsByRange ", bitvector[6])
        print("TestBitVectorSetBitsByRange ", bitvector[7])
        print("TestBitVectorSetBitsByRange ", bitvector[8])
        print("TestBitVectorSetBitsByRange ", bitvector[9])
    } catch (error) {
        print("TestBitVectorSetAllBits failed. code: " + error.code);
    }

    try {
        bitvector.setBitsByRange(0, 6, 8);
        print("TestBitVectorSetBitsByRange ", bitvector[6])
        print("TestBitVectorSetBitsByRange ", bitvector[7])
        print("TestBitVectorSetBitsByRange ", bitvector[8])
    } catch (error) {
        print("TestBitVectorSetAllBits failed. code: " + error.code);
    }

    try {
        bitvector.setBitsByRange(1, 7, 100);
        print("TestBitVectorSetBitsByRange ", bitvector[7])
        print("TestBitVectorSetBitsByRange ", bitvector[8])
    } catch (error) {
        print("setBitsByRange failed. code: " + error.code);
    }

    try {
        bitvector.setBitsByRange(1, 100, 7);
        print("TestBitVectorSetBitsByRange ", bitvector[7])
        print("TestBitVectorSetBitsByRange ", bitvector[8])
    } catch (error) {
        print("setBitsByRange failed. code: " + error.code);
    }

    try {
        bitvector.setBitsByRange(1, -1, 7);
        print("TestBitVectorSetBitsByRange ", bitvector[5])
        print("TestBitVectorSetBitsByRange ", bitvector[6])
    } catch (error) {
        print("setBitsByRange failed. code: " + error.code);
    }

    try {
        bitvector.setBitsByRange(1, -10, -7);
        print("TestBitVectorSetBitsByRange ", bitvector[0])
        print("TestBitVectorSetBitsByRange ", bitvector[1])
    } catch (error) {
        print("setBitsByRange failed. code: " + error.code);
    }

    try {
        bitvector.setBitsByRange('0', 0, 10);
        print("TestBitVectorSetBitsByRange ", bitvector[5])
        print("TestBitVectorSetBitsByRange ", bitvector[6])
    } catch (error) {
        print("setBitsByRange failed. code: " + error.code);
    }

    try {
        bitvector.setBitsByRange('1', 0, 10);
        print("TestBitVectorSetBitsByRange ", bitvector[0])
        print("TestBitVectorSetBitsByRange ", bitvector[1])
    } catch (error) {
        print("setBitsByRange failed. code: " + error.code);
    }
}

function TestBitVectorSetAllBits() {
    let arkPrivate = globalThis.ArkPrivate;
    var BitVector = arkPrivate.Load(arkPrivate.BitVector);
    let bitvector = new BitVector(10);
    bitvector.push(0)
    bitvector.push(1)
    bitvector.push(0)
    bitvector.push(1)
    bitvector.push(0)
    bitvector.push(0)
    bitvector.push(0)
    bitvector.push(1)
    bitvector.push(1)
    bitvector.push(1)
    print("TestBitVectorSetAllBits ", bitvector[0])
    print("TestBitVectorSetAllBits ", bitvector[1])

    try {
        bitvector.setAllBits(1);
        print("TestBitVectorSetAllBits ", bitvector[0])
        print("TestBitVectorSetAllBits ", bitvector[1])
        print("TestBitVectorSetAllBits ", bitvector[9])
    } catch (error) {
        print("TestBitVectorSetAllBits failed. code: " + error.code);
    }

    try {
        bitvector.setAllBits(0);
        print("TestBitVectorSetAllBits ", bitvector[0])
        print("TestBitVectorSetAllBits ", bitvector[1])
        print("TestBitVectorSetAllBits ", bitvector[9])
    } catch (error) {
        print("TestBitVectorSetAllBits failed. code: " + error.code);
    }

    try {
        bitvector.setAllBits(100);
        print("TestBitVectorSetAllBits ", bitvector[0])
        print("TestBitVectorSetAllBits ", bitvector[1])
        print("TestBitVectorSetAllBits ", bitvector[9])
    } catch (error) {
        print("setBitsByRange failed. code: " + error.code);
    }

    try {
        bitvector.setAllBits(-1);
        print("TestBitVectorSetAllBits ", bitvector[0])
        print("TestBitVectorSetAllBits ", bitvector[1])
        print("TestBitVectorSetAllBits ", bitvector[9])
    } catch (error) {
        print("setBitsByRange failed. code: " + error.code);
    }

    try {
        bitvector.setAllBits("111111");
        print("TestBitVectorSetAllBits ", bitvector[0])
        print("TestBitVectorSetAllBits ", bitvector[1])
        print("TestBitVectorSetAllBits ", bitvector[9])
    } catch (error) {
        print("setBitsByRange failed. code: " + error.code);
    }

    try {
        bitvector.setAllBits("1");
        print("TestBitVectorSetAllBits ", bitvector[0])
        print("TestBitVectorSetAllBits ", bitvector[1])
        print("TestBitVectorSetAllBits ", bitvector[9])
    } catch (error) {
        print("setBitsByRange failed. code: " + error.code);
    }

    try {
        bitvector.setAllBits("0");
        print("TestBitVectorSetAllBits ", bitvector[0])
        print("TestBitVectorSetAllBits ", bitvector[1])
        print("TestBitVectorSetAllBits ", bitvector[9])
    } catch (error) {
        print("setBitsByRange failed. code: " + error.code);
    }
}

function TestBitVectorGetBitsByRange() {
    let arkPrivate = globalThis.ArkPrivate;
    var BitVector = arkPrivate.Load(arkPrivate.BitVector);
    let bitvector = new BitVector(10);
    bitvector.push(0)
    bitvector.push(1)
    bitvector.push(0)
    bitvector.push(1)
    bitvector.push(0)
    bitvector.push(0)
    bitvector.push(0)
    bitvector.push(1)
    bitvector.push(1)
    bitvector.push(1)
    print("TestBitVectorGetBitsByRange ", bitvector[0])
    print("TestBitVectorGetBitsByRange ", bitvector[1])

    try {
        let subVec = bitvector.getBitsByRange(2, 8);
        print("TestBitVectorGetBitsByRange ", subVec[0])
        print("TestBitVectorGetBitsByRange ", subVec[1])
        print("TestBitVectorGetBitsByRange ", subVec[2])
        print("TestBitVectorGetBitsByRange ", subVec[3])
        print("TestBitVectorGetBitsByRange ", bitvector[4])
        print("TestBitVectorGetBitsByRange length ", subVec.length)
        print("TestBitVectorGetBitsByRange length ", bitvector.length)
    } catch (error) {
        print("TestBitVectorGetBitsByRange failed. code: " + error.code);
    }
    
    try {
        let subVec1 = bitvector.getBitsByRange(11, 100);
        print("TestBitVectorGetBitsByRange length ", subVec1.length)
    } catch (error) {
        print("TestBitVectorGetBitsByRange failed. code: " + error.code);
    }

    try {
        let subVec1 = bitvector.getBitsByRange(-10, 0);
        print("TestBitVectorGetBitsByRange length ", subVec1.length)
    } catch (error) {
        print("TestBitVectorGetBitsByRange failed. code: " + error.code);
    }

    try {
        let subVec1 = bitvector.getBitsByRange(100, 11);
        print("TestBitVectorGetBitsByRange length ", subVec1.length)
    } catch (error) {
        print("TestBitVectorGetBitsByRange failed. code: " + error.code);
    }

    try {
        let subVec1 = bitvector.getBitsByRange(-10, -1);
        print("TestBitVectorGetBitsByRange length ", subVec1.length)
    } catch (error) {
        print("TestBitVectorGetBitsByRange failed. code: " + error.code);
    }
}

function TestBitVectorResize() {
    let arkPrivate = globalThis.ArkPrivate;
    var BitVector = arkPrivate.Load(arkPrivate.BitVector);
    let bitvector = new BitVector(10);
    bitvector.push(0)
    bitvector.push(1)
    bitvector.push(0)
    bitvector.push(1)
    bitvector.push(0)
    bitvector.push(0)
    bitvector.push(0)
    bitvector.push(1)
    bitvector.push(1)
    bitvector.push(1)
    print("TestBitVectorResize length ", bitvector.length)

    try {
        bitvector.resize(5)
        print("TestBitVectorResize length ", bitvector.length)
    } catch (error) {
        print("TestBitVectorGetBitCountByRange failed. code: " + error.code);
    }

    try {
        bitvector.resize(-1)
        print("TestBitVectorResize length ", bitvector.length)
    } catch (error) {
        print("TestBitVectorResize failed. code: " + error.code);
    }

    try {
        bitvector.resize(0)
        print("TestBitVectorResize length ", bitvector.length)
    } catch (error) {
        print("TestBitVectorResize failed. code: " + error.code);
    }
}

function TestBitVectorGetBitCountByRange() {
    let arkPrivate = globalThis.ArkPrivate;
    var BitVector = arkPrivate.Load(arkPrivate.BitVector);
    let bitvector = new BitVector(10);
    bitvector.push(0)
    bitvector.push(1)
    bitvector.push(0)
    bitvector.push(1)
    bitvector.push(0)
    bitvector.push(0)
    bitvector.push(0)
    bitvector.push(1)
    bitvector.push(1)
    bitvector.push(1)
    print("TestBitVectorGetBitCountByRange has ", bitvector[0])
    print("TestBitVectorGetBitCountByRange has ", bitvector[1])

    let count1 = 0;
    try {
        count1 = bitvector.getBitCountByRange(1, 0, 10);
        print("TestBitVectorGetBitCountByRange count ", count1)
    } catch (error) {
        print("TestBitVectorGetBitCountByRange failed. code: " + error.code);
    }

    try {
        count1 = bitvector.getBitCountByRange(1, -1, 8);
    } catch (error) {
        print("getBitCountByRange failed. code: " + error.code);
    }
    print("TestBitVectorGetBitCountByRange count ", count1)

    count1 = bitvector.getBitCountByRange(0, 0, 10);
    print("TestBitVectorGetBitCountByRange count ", count1)

    try {
        count1 = bitvector.getBitCountByRange(0, -1, 8);
        print("TestBitVectorGetBitCountByRange count ", count1)
    } catch (error) {
        print("getBitCountByRange failed. code: " + error.code);
    }

    try {
        count1 = bitvector.getBitCountByRange(0, -8, -1);
        print("TestBitVectorGetBitCountByRange count ", count1)
    } catch (error) {
        print("getBitCountByRange failed. code: " + error.code);
    }

    try {
        count1 = bitvector.getBitCountByRange(0, -1, -10);
        print("TestBitVectorGetBitCountByRange count ", count1)
    } catch (error) {
        print("getBitCountByRange failed. code: " + error.code);
    }

    try {
        count1 = bitvector.getBitCountByRange('0', 0, 10);
        print("TestBitVectorGetBitCountByRange count ", count1)
    } catch (error) {
        print("getBitCountByRange failed. code: " + error.code);
    }

    try {
        count1 = bitvector.getBitCountByRange('1', 0, 10);
        print("TestBitVectorGetBitCountByRange count ", count1)
    } catch (error) {
        print("getBitCountByRange failed. code: " + error.code);
    }
}

function TestBitVectorGetIndexOf() {
    let arkPrivate = globalThis.ArkPrivate;
    var BitVector = arkPrivate.Load(arkPrivate.BitVector);
    let bitvector = new BitVector(10);
    bitvector.push(0)
    bitvector.push(1)
    bitvector.push(0)
    bitvector.push(1)
    bitvector.push(0)
    bitvector.push(0)
    bitvector.push(0)
    bitvector.push(1)
    bitvector.push(1)
    bitvector.push(1)
    print("TestBitVectorGetIndexOf has ", bitvector[0])
    print("TestBitVectorGetIndexOf has ", bitvector[1])

    let num = 0;
    try {
        num = bitvector.getIndexOf(1, 0, 10);
        print("TestBitVectorGetIndexOf count ", num)
    } catch (error) {
        print("TestBitVectorGetLastIndexOf failed. code: " + error.code);
    }

    try {
        num = bitvector.getIndexOf(1, -1, 10);
        print("TestBitVectorGetIndexOf count ", num)
    } catch (error) {
        print("TestBitVectorGetIndexOf failed. code: " + error.code);
    }

    try {
        num = bitvector.getIndexOf(1, 8, 1);
        print("TestBitVectorGetIndexOf count ", num)
    } catch (error) {
        print("TestBitVectorGetIndexOf failed. code: " + error.code);
    }

    try {
        num = bitvector.getIndexOf(1, 0, 100);
        print("TestBitVectorGetIndexOf count ", num)
    } catch (error) {
        print("TestBitVectorGetIndexOf failed. code: " + error.code);
    }

    try {
        num = bitvector.getIndexOf(0, -1, 10);
        print("TestBitVectorGetIndexOf count ", num)
    } catch (error) {
        print("TestBitVectorGetIndexOf failed. code: " + error.code);
    }

    try {
        num = bitvector.getIndexOf(0, 8, 1);
        print("TestBitVectorGetIndexOf count ", num)
    } catch (error) {
        print("TestBitVectorGetIndexOf failed. code: " + error.code);
    }

    try {
        num = bitvector.getIndexOf(0, 0, 100);
        print("TestBitVectorGetIndexOf count ", num)
    } catch (error) {
        print("TestBitVectorGetIndexOf failed. code: " + error.code);
    }

    try {
        num = bitvector.getIndexOf('0', 1, 10);
        print("TestBitVectorGetIndexOf count ", num)
    } catch (error) {
        print("TestBitVectorGetIndexOf failed. code: " + error.code);
    }

    try {
        num = bitvector.getIndexOf('1', 0, 10);
        print("TestBitVectorGetIndexOf count ", num)
    } catch (error) {
        print("TestBitVectorGetIndexOf failed. code: " + error.code);
    }
}

function TestBitVectorGetLastIndexOf() {
    let arkPrivate = globalThis.ArkPrivate;
    var BitVector = arkPrivate.Load(arkPrivate.BitVector);
    let bitvector = new BitVector(10);
    bitvector.push(0)
    bitvector.push(1)
    bitvector.push(0)
    bitvector.push(1)
    bitvector.push(0)
    bitvector.push(0)
    bitvector.push(0)
    bitvector.push(1)
    bitvector.push(1)
    bitvector.push(1)
    print("TestBitVectorGetLastIndexOf has ", bitvector[0])
    print("TestBitVectorGetLastIndexOf has ", bitvector[1])

    let num = 0;
    try {
        num = bitvector.getLastIndexOf(1, 0, 10);
        print("TestBitVectorGetLastIndexOf count ", num)
    } catch (error) {
        print("TestBitVectorGetLastIndexOf failed. code: " + error.code);
    }
    
    try {
        num = bitvector.getLastIndexOf(1, -1, 10);
        print("TestBitVectorGetLastIndexOf count ", num)
    } catch (error) {
        print("TestBitVectorGetIndexOf failed. code: " + error.code);
    }

    try {
        num = bitvector.getLastIndexOf(1, 8, 1);
        print("TestBitVectorGetLastIndexOf count ", num)
    } catch (error) {
        print("TestBitVectorGetIndexOf failed. code: " + error.code);
    }

    try {
        num = bitvector.getLastIndexOf(1, 0, 100);
        print("TestBitVectorGetLastIndexOf count ", num)
    } catch (error) {
        print("TestBitVectorGetIndexOf failed. code: " + error.code);
    }

    try {
        num = bitvector.getLastIndexOf(0, -1, 10);
        print("TestBitVectorGetLastIndexOf count ", num)
    } catch (error) {
        print("TestBitVectorGetIndexOf failed. code: " + error.code);
    }

    try {
        num = bitvector.getLastIndexOf(0, 8, 1);
        print("TestBitVectorGetLastIndexOf count ", num)
    } catch (error) {
        print("TestBitVectorGetIndexOf failed. code: " + error.code);
    }

    try {
        num = bitvector.getLastIndexOf(0, 0, 100);
        print("TestBitVectorGetLastIndexOf count ", num)
    } catch (error) {
        print("TestBitVectorGetIndexOf failed. code: " + error.code);
    }

    try {
        num = bitvector.getLastIndexOf(100, 0, 100);
        print("TestBitVectorGetLastIndexOf count ", num)
    } catch (error) {
        print("TestBitVectorGetIndexOf failed. code: " + error.code);
    }

    try {
        num = bitvector.getLastIndexOf("111", 0, 100);
        print("TestBitVectorGetLastIndexOf count ", num)
    } catch (error) {
        print("TestBitVectorGetIndexOf failed. code: " + error.code);
    }

    try {
        num = bitvector.getLastIndexOf("0", 0, 100);
        print("TestBitVectorGetLastIndexOf count ", num)
    } catch (error) {
        print("TestBitVectorGetIndexOf failed. code: " + error.code);
    }

    try {
        num = bitvector.getLastIndexOf("1", 0, 100);
        print("TestBitVectorGetLastIndexOf count ", num)
    } catch (error) {
        print("TestBitVectorGetIndexOf failed. code: " + error.code);
    }
}

function TestBitVectorFlipBitByIndex() {
    let arkPrivate = globalThis.ArkPrivate;
    var BitVector = arkPrivate.Load(arkPrivate.BitVector);
    let bitvector = new BitVector(10);
    bitvector.push(0)
    bitvector.push(1)
    bitvector.push(0)
    bitvector.push(1)
    bitvector.push(0)
    bitvector.push(0)
    bitvector.push(0)
    bitvector.push(1)
    bitvector.push(1)
    bitvector.push(1)
    print("TestBitVectorFlipBitByIndex ", bitvector[0])
    print("TestBitVectorFlipBitByIndex ", bitvector[1])

    try {
        bitvector.flipBitByIndex(0);
        print("TestBitVectorFlipBitByIndex ", bitvector[0])
    } catch (error) {
        print("TestBitVectorFlipBitByIndex failed. code: " + error.code);
    }

    try {
        bitvector.flipBitByIndex(1);
        print("TestBitVectorFlipBitByIndex ", bitvector[1])
    } catch (error) {
        print("TestBitVectorFlipBitByIndex failed. code: " + error.code);
    }

    try {
        bitvector.flipBitByIndex(100);
    } catch (error) {
        print("TestBitVectorFlipBitByIndex failed. code: " + error.code);
    }
    
    try {
        bitvector.flipBitByIndex(-1);
    } catch (error) {
        print("TestBitVectorFlipBitByIndex failed. code: " + error.code);
    }

    try {
        bitvector.flipBitByIndex(10);
    } catch (error) {
        print("TestBitVectorFlipBitByIndex failed. code: " + error.code);
    }

    try {
        bitvector.flipBitByIndex(-10);
    } catch (error) {
        print("TestBitVectorFlipBitByIndex failed. code: " + error.code);
    }

    try {
        bitvector.flipBitByIndex('0');
    } catch (error) {
        print("TestBitVectorFlipBitByIndex failed. code: " + error.code);
    }

    try {
        bitvector.flipBitByIndex('1');
    } catch (error) {
        print("TestBitVectorFlipBitByIndex failed. code: " + error.code);
    }
}

function TestBitVectorFlipBitByRange() {
    let arkPrivate = globalThis.ArkPrivate;
    var BitVector = arkPrivate.Load(arkPrivate.BitVector);
    let bitvector = new BitVector(10);
    bitvector.push(0)
    bitvector.push(1)
    bitvector.push(0)
    bitvector.push(1)
    bitvector.push(0)
    bitvector.push(0)
    bitvector.push(0)
    bitvector.push(1)
    bitvector.push(1)
    bitvector.push(1)
    print("TestBitVectorFlipBitByRange ", bitvector[0])
    print("TestBitVectorFlipBitByRange ", bitvector[1])

    try {
        bitvector.flipBitsByRange(0, 10);
        print("TestBitVectorFlipBitByRange ", bitvector[0])
        print("TestBitVectorFlipBitByRange ", bitvector[1])
    } catch (error) {
        print("TestBitVectorFlipBitByRange failed. code: " + error.code);
    }

    try {
        bitvector.flipBitsByRange(1, 9);
        print("TestBitVectorFlipBitByRange ", bitvector[0])
        print("TestBitVectorFlipBitByRange ", bitvector[1])
        print("TestBitVectorFlipBitByRange ", bitvector[8])
        print("TestBitVectorFlipBitByRange ", bitvector[9])
    } catch (error) {
        print("TestBitVectorFlipBitByRange failed. code: " + error.code);
    }

    try {
        bitvector.flipBitsByRange(-1, 100);
        print("TestBitVectorFlipBitByRange ", bitvector[0])
        print("TestBitVectorFlipBitByRange ", bitvector[1])
        print("TestBitVectorFlipBitByRange ", bitvector[9])
    } catch (error) {
        print("TestBitVectorFlipBitByRange failed. code: " + error.code);
    }

    try {
        bitvector.flipBitsByRange(-10, 1);
        print("TestBitVectorFlipBitByRange ", bitvector[0])
        print("TestBitVectorFlipBitByRange ", bitvector[1])
        print("TestBitVectorFlipBitByRange ", bitvector[9])
    } catch (error) {
        print("TestBitVectorFlipBitByRange failed. code: " + error.code);
    }

    try {
        bitvector.flipBitsByRange(-10, -1);
        print("TestBitVectorFlipBitByRange ", bitvector[0])
        print("TestBitVectorFlipBitByRange ", bitvector[1])
        print("TestBitVectorFlipBitByRange ", bitvector[9])
    } catch (error) {
        print("TestBitVectorFlipBitByRange failed. code: " + error.code);
    }

    try {
        bitvector.flipBitsByRange(-1, 0);
        print("TestBitVectorFlipBitByRange ", bitvector[0])
        print("TestBitVectorFlipBitByRange ", bitvector[1])
        print("TestBitVectorFlipBitByRange ", bitvector[9])
    } catch (error) {
        print("TestBitVectorFlipBitByRange failed. code: " + error.code);
    }

    try {
        bitvector.flipBitsByRange(0, 0);
        print("TestBitVectorFlipBitByRange ", bitvector[0])
        print("TestBitVectorFlipBitByRange ", bitvector[1])
        print("TestBitVectorFlipBitByRange ", bitvector[9])
    } catch (error) {
        print("TestBitVectorFlipBitByRange failed. code: " + error.code);
    }
}

function TestBitVectorValues() {
    let arkPrivate = globalThis.ArkPrivate;
    var BitVector = arkPrivate.Load(arkPrivate.BitVector);
    let bitvector = new BitVector(10);
    bitvector.push(0)
    bitvector.push(1)
    bitvector.push(0)
    bitvector.push(1)
    bitvector.push(0)
    bitvector.push(0)
    bitvector.push(0)
    bitvector.push(1)
    bitvector.push(1)
    bitvector.push(1)

    const iterator = bitvector.values();
    for (const value of iterator) {
        print("TestBitVectorValues " + value);
    }
}

newsharedclassfrom()
TestBitVector()
TestBitVectorPop()
TestBitVectorHas()
TestBitVectorSetBitsByRange()
TestBitVectorSetAllBits()
TestBitVectorGetBitsByRange()
TestBitVectorResize()
TestBitVectorGetBitCountByRange()
TestBitVectorGetIndexOf()
TestBitVectorGetLastIndexOf()
TestBitVectorFlipBitByIndex()
TestBitVectorFlipBitByRange()
TestBitVectorValues()
