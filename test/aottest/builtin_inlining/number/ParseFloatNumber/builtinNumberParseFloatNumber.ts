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

declare function print(arg:any):string;

function testNumberParseFloat(a : any, shouldThrow = 0): any
{
    try {
        if (shouldThrow == 1) {
            throw Error("thr 1");
        }
        let x = Number.parseFloat(a);
        if (shouldThrow == 2) {
            throw Error("thr 2");
        }
        return x;
    } catch (e) {
        print("catch", "'" + e + "'", "in testNumberParseFloat");
        throw (e)
    } finally {
        print("exit testNumberParseFloat");
    }
}

function testParseFloat(a : any, shouldThrow = 0): any
{
    try {
        if (shouldThrow == 1) {
            throw Error("thr 1");
        }
        let x = parseFloat(a);
        if (shouldThrow == 2) {
            throw Error("thr 2");
        }
        return x;
    } catch (e) {
        print("catch", "'" + e + "'", "in testParseFloat");
        throw (e)
    } finally {
        print("exit testParseFloat");
    }
}

print(Number.parseFloat === parseFloat);
//: true

function test(a : any)
{
    let checks = [typeof a, a];
    print(checks)
}

function testParseFloatInt(a: number) {
    try {
        return parseFloat((a | 0) as any);
    } catch (e) {
        print("catch", "'" + e + "'", "in testParseFloat");
        throw (e)
    }
}
test(testParseFloatInt(-2));
//aot: [trace] aot inline builtin: Number.parseFloat, caller function name:#*#testParseFloatInt@builtinNumberParseFloatNumber
//aot: [trace] aot inline function name: #*#test@builtinNumberParseFloatNumber caller function name: func_main_0@builtinNumberParseFloatNumber
//: number,-2

let negZero = testParseFloat(-0.0 as any);
//aot: [trace] aot inline builtin: Number.parseFloat, caller function name:#*#testParseFloat@builtinNumberParseFloatNumber
//aot: [trace] Check Type: NotString1
//: exit testParseFloat
print(Object.is(negZero, -0))
//aot: [trace] aot inline builtin: Object.is, caller function name:func_main_0@builtinNumberParseFloatNumber
//: false
print(Object.is(negZero, 0))
//aot: [trace] aot inline builtin: Object.is, caller function name:func_main_0@builtinNumberParseFloatNumber
//: true

test(testParseFloat(-2.01));
//aot: [trace] aot inline builtin: Number.parseFloat, caller function name:#*#testParseFloat@builtinNumberParseFloatNumber
//aot: [trace] Check Type: NotString1
//: exit testParseFloat
//aot: [trace] aot inline function name: #*#test@builtinNumberParseFloatNumber caller function name: func_main_0@builtinNumberParseFloatNumber
//: number,-2.01

test(testNumberParseFloat(-2.01));
//aot: [trace] aot inline builtin: Number.parseFloat, caller function name:#*#testNumberParseFloat@builtinNumberParseFloatNumber
//aot: [trace] Check Type: NotString1
//: exit testNumberParseFloat
//aot: [trace] aot inline function name: #*#test@builtinNumberParseFloatNumber caller function name: func_main_0@builtinNumberParseFloatNumber
//: number,-2.01
