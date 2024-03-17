/*
 * Copyright (c) 2023 Huawei Device Co., Ltd.
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
 * @tc.name:BigInt
 * @tc.desc:test BigInt
 * @tc.type: FUNC
 * @tc.require: issueI7DG0J
 */

var number = Number(0x01000000000000080000000000000000000000n)
print(number)

try {
    BigInt.asUintN(2147483649, -1873965464n);
} catch (e) {
    print("test successful");
}

const n1 = Number(0x1fffeffafbfcfen);
print(n1);

try {
    print((9007199254740991n + 9007199254740991n) ** 0n);
    print(0n ** (9007199254740991n + 9007199254740991n));
    print((9007199254740991n + 9007199254740991n) ** 9007199254740991n);
} catch (err) {
    print(err.name);
}

try {
    print(0x4141414141414141n << 0n);
    print(0n << 0x4141414141414141n);
    print(0x4141414141414141n << 0x4141414141414141n);
} catch (err) {
    print(err.name);
}

try {
    const atom = this.Atomics;
    atom.and(new BigInt64Array(3807), atom, atom);
} catch (err) {
    print(err.name)
}

const v33 = String.fromCharCode(48).padStart(48, String.fromCharCode(48));
const v35 = [-1073741824, 2, -9007199254740992];
const v42 = Symbol.toPrimitive;
Symbol.for(v42.description);
function f36(a37, a38) {
    try {
        a37.lastIndexOf(0);
    } catch (err) {
        print(err);
    }
    return v33;
}
v35[v42] = f36;
let v49 = -12n;
v49--;
const v52 = (v49 >> v49).constructor;
const t48 = v52.__defineSetter__;
print(v52(v35));

let map = new Map();
let a = BigInt(9007199254740991);
let b = BigInt(0x1fffffffffffff);
let c = BigInt(0b11111111111111111111111111111111111111111111111111111);
let d = BigInt(9007199254740991);
map.set(a, '1');
map.set(b, '2');
map.set(c, '3');
map.set(d, '4');
map.forEach((val,key)=>{
    print("map val:" + val, "key :" + key);
})

var HashMap = ArkPrivate.Load(ArkPrivate.HashMap);
let hmap = new HashMap();
hmap.set(a, '1');
hmap.set(b, '2');
hmap.set(c, '3');
hmap.set(d, '4');
hmap.forEach((val,key)=>{
    print("hmap val:" + val, "key :" + key);
})

const v83 = new BigUint64Array(1096);
try {
    v83["toLocaleString"]("toLocaleString", "toLocaleString");
} catch (error) {
    print(error);
}

print(BigInt.asUintN(2**32, 42n));
print(BigInt.asIntN(2**32, 42n));
print((1n)<<(61n)>>(61n));
