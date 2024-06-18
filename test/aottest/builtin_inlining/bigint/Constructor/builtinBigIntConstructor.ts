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

declare interface ArkTools {
    isAOTCompiled(args: any): boolean;
}
declare function print(arg:any):string;
function replace(a)
{
    return a * 2;
}

function checkBigInt(x) {
    let val = BigInt(x);
    print(val);
    if (typeof val !== 'bigint') {
        print(typeof val);
        throw new Error();
    }
}

function checkNotBigInt(x) {
    try {
        let val = BigInt(x);
        print(val);
        print(typeof val);
    } catch (e) {
        print(e.name)
    }
}


//aot: [trace] aot inline builtin: BigInt, caller function name:func_main_0@builtinBigIntConstructor
let n = BigInt(1);
print(typeof n); //: bigint
//aot: [trace] aot inline builtin: BigInt, caller function name:func_main_0@builtinBigIntConstructor
n = BigInt(1)
print(n); //: 1

//aot: [trace] aot inline builtin: BigInt, caller function name:func_main_0@builtinBigIntConstructor
n = BigInt(1, 2)
print(typeof n); //: bigint
print(n); //: 1
//aot: [trace] aot inline function name: #*#checkBigInt@builtinBigIntConstructor caller function name: func_main_0@builtinBigIntConstructor
//aot: [trace] aot inline builtin: BigInt, caller function name:#*#checkBigInt@builtinBigIntConstructor
checkBigInt(-1); //: -1
//aot: [trace] aot inline function name: #*#checkBigInt@builtinBigIntConstructor caller function name: func_main_0@builtinBigIntConstructor
//aot: [trace] aot inline builtin: BigInt, caller function name:#*#checkBigInt@builtinBigIntConstructor
checkBigInt(-12345); //: -12345
//aot: [trace] aot inline function name: #*#checkBigInt@builtinBigIntConstructor caller function name: func_main_0@builtinBigIntConstructor
//aot: [trace] aot inline builtin: BigInt, caller function name:#*#checkBigInt@builtinBigIntConstructor
checkBigInt(1n); //: 1
//aot: [trace] aot inline function name: #*#checkBigInt@builtinBigIntConstructor caller function name: func_main_0@builtinBigIntConstructor
//aot: [trace] aot inline builtin: BigInt, caller function name:#*#checkBigInt@builtinBigIntConstructor
checkBigInt(-1n); //: -1
//aot: [trace] aot inline function name: #*#checkBigInt@builtinBigIntConstructor caller function name: func_main_0@builtinBigIntConstructor
//aot: [trace] aot inline builtin: BigInt, caller function name:#*#checkBigInt@builtinBigIntConstructor
checkBigInt(0.0); //: 0
//aot: [trace] aot inline function name: #*#checkBigInt@builtinBigIntConstructor caller function name: func_main_0@builtinBigIntConstructor
//aot: [trace] aot inline builtin: BigInt, caller function name:#*#checkBigInt@builtinBigIntConstructor
checkBigInt(-0.0); //: 0
//aot: [trace] aot inline builtin: BigInt, caller function name:func_main_0@builtinBigIntConstructor
//aot: [trace] aot inline builtin: BigInt, caller function name:#*#checkBigInt@builtinBigIntConstructor
checkBigInt(BigInt(-1)); //: -1

// test large ints
// 2^31-1
//aot: [trace] aot inline builtin: BigInt, caller function name:#*#checkBigInt@builtinBigIntConstructor
checkBigInt(2147483647); //: 2147483647
//aot: [trace] aot inline builtin: BigInt, caller function name:#*#checkBigInt@builtinBigIntConstructor
checkBigInt(2147483647 << 1); //: -2
//-(2^32)
//aot: [trace] aot inline builtin: BigInt, caller function name:#*#checkBigInt@builtinBigIntConstructor
checkBigInt(-2147483648); //: -2147483648
//aot: [trace] aot inline builtin: BigInt, caller function name:#*#checkBigInt@builtinBigIntConstructor
checkBigInt(Number.MIN_SAFE_INTEGER); //: -9007199254740991
//aot: [trace] aot inline builtin: BigInt, caller function name:#*#checkBigInt@builtinBigIntConstructor
checkBigInt(Number.MAX_SAFE_INTEGER); //: 9007199254740991
//aot: [trace] aot inline builtin: BigInt, caller function name:#*#checkBigInt@builtinBigIntConstructor
checkBigInt(Number.MAX_SAFE_INTEGER - 0.5); //: 9007199254740990
// 1e22
//aot: [trace] aot inline builtin: BigInt, caller function name:#*#checkBigInt@builtinBigIntConstructor
checkBigInt(10000000000000000000000) //: 10000000000000000000000

//aot: [trace] aot inline builtin: BigInt, caller function name:#*#checkBigInt@builtinBigIntConstructor
checkBigInt(1e23) //: 99999999999999991611392
// also 1e23
//aot: [trace] aot inline builtin: BigInt, caller function name:#*#checkBigInt@builtinBigIntConstructor
checkBigInt(100000000000000000000000) //: 99999999999999991611392
//aot: [trace] aot inline builtin: BigInt, caller function name:#*#checkBigInt@builtinBigIntConstructor
checkBigInt("100000000000000000000000") //: 100000000000000000000000
//aot: [trace] aot inline builtin: BigInt, caller function name:#*#checkBigInt@builtinBigIntConstructor
checkBigInt("-100000000000000000000000") //: -100000000000000000000000
//aot: [trace] aot inline builtin: BigInt, caller function name:#*#checkBigInt@builtinBigIntConstructor
checkBigInt(100000000000000000000000n) //: 100000000000000000000000
//aot: [trace] aot inline builtin: BigInt, caller function name:func_main_0@builtinBigIntConstructor
//aot: [trace] aot inline builtin: BigInt, caller function name:#*#checkBigInt@builtinBigIntConstructor
checkBigInt(BigInt(100000000000000000000000n)) //: 100000000000000000000000


//aot: [trace] aot inline builtin: BigInt, caller function name:#*#checkBigInt@builtinBigIntConstructor
checkBigInt("0xf") //: 15
//aot: [trace] aot inline builtin: BigInt, caller function name:#*#checkBigInt@builtinBigIntConstructor
checkBigInt({ valueOf: () => 1 }); //: 1
//aot: [trace] aot inline builtin: BigInt, caller function name:#*#checkBigInt@builtinBigIntConstructor
checkBigInt({ valueOf: () => "-11" }); //: -11

//aot: [trace] aot inline builtin: BigInt, caller function name:#*#checkNotBigInt@builtinBigIntConstructor
checkNotBigInt(1.5); //: RangeError
//aot: [trace] aot inline builtin: BigInt, caller function name:#*#checkNotBigInt@builtinBigIntConstructor
checkNotBigInt(-20.000001); //: RangeError
//aot: [trace] aot inline builtin: BigInt, caller function name:#*#checkNotBigInt@builtinBigIntConstructor
checkNotBigInt(Number.EPSILON); //: RangeError
//aot: [trace] aot inline builtin: BigInt, caller function name:#*#checkNotBigInt@builtinBigIntConstructor
checkNotBigInt(Number.POSITIVE_INFINITY); //: RangeError
//aot: [trace] aot inline builtin: BigInt, caller function name:#*#checkNotBigInt@builtinBigIntConstructor
checkNotBigInt(Number.NEGATIVE_INFINITY); //: RangeError
//aot: [trace] aot inline builtin: BigInt, caller function name:#*#checkNotBigInt@builtinBigIntConstructor
checkNotBigInt(NaN); //: RangeError

//aot: [trace] aot inline builtin: BigInt, caller function name:#*#checkNotBigInt@builtinBigIntConstructor
checkNotBigInt(null); //: TypeError
//aot: [trace] aot inline builtin: BigInt, caller function name:#*#checkNotBigInt@builtinBigIntConstructor
checkNotBigInt(undefined); //: TypeError
//aot: [trace] aot inline builtin: BigInt, caller function name:#*#checkNotBigInt@builtinBigIntConstructor
checkNotBigInt("f"); //: SyntaxError
//aot: [trace] aot inline builtin: BigInt, caller function name:#*#checkNotBigInt@builtinBigIntConstructor
checkNotBigInt({}); //: SyntaxError

try {
    //NOTE: not inlined by now
    //aot: [trace] aot inline builtin: BigInt, caller function name:func_main_0@builtinBigIntConstructor
    n = BigInt();
} catch(e) {
    print(e.name); //: TypeError
}

try {
    // not inlined as expected
    n = new BigInt(1);
} catch(e) {
    print(e.name); //: TypeError
}

//aot: [trace] aot inline builtin: BigInt, caller function name:func_main_0@builtinBigIntConstructor
n = 2n.constructor(12);
print(typeof n); //: bigint
print(n); //: 12

//aot: [trace] aot inline builtin: BigInt, caller function name:func_main_0@builtinBigIntConstructor
n = globalThis.BigInt(12);
print(typeof n); //: bigint
print(n); //: 12

if (ArkTools.isAOTCompiled(checkBigInt)) {
    BigInt = replace;
}
//aot: [trace] Check Type: NotCallTarget1
print(BigInt(2)); //pgo: 2
                  //aot: 4
