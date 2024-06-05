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

function valueTest(testFunc : any) {
    test(testFunc("12"));
    test(testFunc("1.2"));
    test(testFunc(".2"));
    test(testFunc("-.2"));
    test(testFunc("2."));
    test(testFunc("-2."));
    test(testFunc("  \t \n-.2"));
    test(testFunc("-.2+2"));
    test(testFunc("12e-2"));
    test(testFunc("0.012E+2"));
    test(testFunc("."));
    test(testFunc("1.7976931348623159e+308"));
    test(testFunc("Infinity"));
    test(testFunc("-1.7976931348623159e+308"));
    test(testFunc("-Infinity"));
    test(testFunc("900719925474099267n"));
}

valueTest(testParseFloat)
//aot: [trace] aot inline builtin: Number.parseFloat, caller function name:#*#testParseFloat@builtinNumberParseFloat
//: exit testParseFloat
//aot: [trace] aot inline function name: #*#test@builtinNumberParseFloat caller function name: #*#valueTest@builtinNumberParseFloat
//: number,12
//aot: [trace] aot inline builtin: Number.parseFloat, caller function name:#*#testParseFloat@builtinNumberParseFloat
//: exit testParseFloat
//aot: [trace] aot inline function name: #*#test@builtinNumberParseFloat caller function name: #*#valueTest@builtinNumberParseFloat
//: number,1.2
//aot: [trace] aot inline builtin: Number.parseFloat, caller function name:#*#testParseFloat@builtinNumberParseFloat
//: exit testParseFloat
//aot: [trace] aot inline function name: #*#test@builtinNumberParseFloat caller function name: #*#valueTest@builtinNumberParseFloat
//: number,0.2
//aot: [trace] aot inline builtin: Number.parseFloat, caller function name:#*#testParseFloat@builtinNumberParseFloat
//: exit testParseFloat
//aot: [trace] aot inline function name: #*#test@builtinNumberParseFloat caller function name: #*#valueTest@builtinNumberParseFloat
//: number,-0.2
//aot: [trace] aot inline builtin: Number.parseFloat, caller function name:#*#testParseFloat@builtinNumberParseFloat
//: exit testParseFloat
//aot: [trace] aot inline function name: #*#test@builtinNumberParseFloat caller function name: #*#valueTest@builtinNumberParseFloat
//: number,2
//aot: [trace] aot inline builtin: Number.parseFloat, caller function name:#*#testParseFloat@builtinNumberParseFloat
//: exit testParseFloat
//aot: [trace] aot inline function name: #*#test@builtinNumberParseFloat caller function name: #*#valueTest@builtinNumberParseFloat
//: number,-2
//aot: [trace] aot inline builtin: Number.parseFloat, caller function name:#*#testParseFloat@builtinNumberParseFloat
//: exit testParseFloat
//: number,-0.2
//aot: [trace] aot inline builtin: Number.parseFloat, caller function name:#*#testParseFloat@builtinNumberParseFloat
//: exit testParseFloat
//: number,-0.2
//aot: [trace] aot inline builtin: Number.parseFloat, caller function name:#*#testParseFloat@builtinNumberParseFloat
//: exit testParseFloat
//: number,0.12
//aot: [trace] aot inline builtin: Number.parseFloat, caller function name:#*#testParseFloat@builtinNumberParseFloat
//: exit testParseFloat
//: number,1.2
//aot: [trace] aot inline builtin: Number.parseFloat, caller function name:#*#testParseFloat@builtinNumberParseFloat
//: exit testParseFloat
//: number,NaN
//aot: [trace] aot inline builtin: Number.parseFloat, caller function name:#*#testParseFloat@builtinNumberParseFloat
//: exit testParseFloat
//: number,Infinity
//aot: [trace] aot inline builtin: Number.parseFloat, caller function name:#*#testParseFloat@builtinNumberParseFloat
//: exit testParseFloat
//: number,Infinity
//aot: [trace] aot inline builtin: Number.parseFloat, caller function name:#*#testParseFloat@builtinNumberParseFloat
//: exit testParseFloat
//: number,-Infinity
//aot: [trace] aot inline builtin: Number.parseFloat, caller function name:#*#testParseFloat@builtinNumberParseFloat
//: exit testParseFloat
//: number,-Infinity
//aot: [trace] aot inline builtin: Number.parseFloat, caller function name:#*#testParseFloat@builtinNumberParseFloat
//: exit testParseFloat
//: number,900719925474099300

valueTest(testNumberParseFloat)
//aot: [trace] aot inline builtin: Number.parseFloat, caller function name:#*#testNumberParseFloat@builtinNumberParseFloat
//: exit testNumberParseFloat
//aot: [trace] aot inline function name: #*#test@builtinNumberParseFloat caller function name: #*#valueTest@builtinNumberParseFloat
//: number,12
//aot: [trace] aot inline builtin: Number.parseFloat, caller function name:#*#testNumberParseFloat@builtinNumberParseFloat
//: exit testNumberParseFloat
//aot: [trace] aot inline function name: #*#test@builtinNumberParseFloat caller function name: #*#valueTest@builtinNumberParseFloat
//: number,1.2
//aot: [trace] aot inline builtin: Number.parseFloat, caller function name:#*#testNumberParseFloat@builtinNumberParseFloat
//: exit testNumberParseFloat
//aot: [trace] aot inline function name: #*#test@builtinNumberParseFloat caller function name: #*#valueTest@builtinNumberParseFloat
//: number,0.2
//aot: [trace] aot inline builtin: Number.parseFloat, caller function name:#*#testNumberParseFloat@builtinNumberParseFloat
//: exit testNumberParseFloat
//aot: [trace] aot inline function name: #*#test@builtinNumberParseFloat caller function name: #*#valueTest@builtinNumberParseFloat
//: number,-0.2
//aot: [trace] aot inline builtin: Number.parseFloat, caller function name:#*#testNumberParseFloat@builtinNumberParseFloat
//: exit testNumberParseFloat
//aot: [trace] aot inline function name: #*#test@builtinNumberParseFloat caller function name: #*#valueTest@builtinNumberParseFloat
//: number,2
//aot: [trace] aot inline builtin: Number.parseFloat, caller function name:#*#testNumberParseFloat@builtinNumberParseFloat
//: exit testNumberParseFloat
//aot: [trace] aot inline function name: #*#test@builtinNumberParseFloat caller function name: #*#valueTest@builtinNumberParseFloat
//: number,-2
//aot: [trace] aot inline builtin: Number.parseFloat, caller function name:#*#testNumberParseFloat@builtinNumberParseFloat
//: exit testNumberParseFloat
//: number,-0.2
//aot: [trace] aot inline builtin: Number.parseFloat, caller function name:#*#testNumberParseFloat@builtinNumberParseFloat
//: exit testNumberParseFloat
//: number,-0.2
//aot: [trace] aot inline builtin: Number.parseFloat, caller function name:#*#testNumberParseFloat@builtinNumberParseFloat
//: exit testNumberParseFloat
//: number,0.12
//aot: [trace] aot inline builtin: Number.parseFloat, caller function name:#*#testNumberParseFloat@builtinNumberParseFloat
//: exit testNumberParseFloat
//: number,1.2
//aot: [trace] aot inline builtin: Number.parseFloat, caller function name:#*#testNumberParseFloat@builtinNumberParseFloat
//: exit testNumberParseFloat
//: number,NaN
//aot: [trace] aot inline builtin: Number.parseFloat, caller function name:#*#testNumberParseFloat@builtinNumberParseFloat
//: exit testNumberParseFloat
//: number,Infinity
//aot: [trace] aot inline builtin: Number.parseFloat, caller function name:#*#testNumberParseFloat@builtinNumberParseFloat
//: exit testNumberParseFloat
//: number,Infinity
//aot: [trace] aot inline builtin: Number.parseFloat, caller function name:#*#testNumberParseFloat@builtinNumberParseFloat
//: exit testNumberParseFloat
//: number,-Infinity
//aot: [trace] aot inline builtin: Number.parseFloat, caller function name:#*#testNumberParseFloat@builtinNumberParseFloat
//: exit testNumberParseFloat
//: number,-Infinity
//aot: [trace] aot inline builtin: Number.parseFloat, caller function name:#*#testNumberParseFloat@builtinNumberParseFloat
//: exit testNumberParseFloat
//: number,900719925474099300

// Check deopt:
let obj = {};
obj.toString = function () : string {
    return "22";
}
test(testParseFloat(obj));
//aot: [trace] aot inline builtin: Number.parseFloat, caller function name:#*#testParseFloat@builtinNumberParseFloat
//aot: [trace] Check Type: NotString1
//: exit testParseFloat
//aot: [trace] aot inline function name: #*#test@builtinNumberParseFloat caller function name: func_main_0@builtinNumberParseFloat
//: number,22
test(testNumberParseFloat(obj));
//aot: [trace] aot inline builtin: Number.parseFloat, caller function name:#*#testNumberParseFloat@builtinNumberParseFloat
//aot: [trace] Check Type: NotString1
//: exit testNumberParseFloat
//aot: [trace] aot inline function name: #*#test@builtinNumberParseFloat caller function name: func_main_0@builtinNumberParseFloat
//: number,22

obj = new Number(22);
test(testParseFloat(obj));
//aot: [trace] aot inline builtin: Number.parseFloat, caller function name:#*#testParseFloat@builtinNumberParseFloat
//aot: [trace] Check Type: NotString1
//: exit testParseFloat
//aot: [trace] aot inline function name: #*#test@builtinNumberParseFloat caller function name: func_main_0@builtinNumberParseFloat
//: number,22
test(testNumberParseFloat(obj));
//aot: [trace] aot inline builtin: Number.parseFloat, caller function name:#*#testNumberParseFloat@builtinNumberParseFloat
//aot: [trace] Check Type: NotString1
//: exit testNumberParseFloat
//aot: [trace] aot inline function name: #*#test@builtinNumberParseFloat caller function name: func_main_0@builtinNumberParseFloat
//: number,22

// Check deopt with throw:
try {
    test(testParseFloat(obj, 1));
} catch (e) {}
//: catch 'Error: thr 1' in testParseFloat
//: exit testParseFloat
try {
    test(testNumberParseFloat(obj, 1));
} catch (e) {}
//: catch 'Error: thr 1' in testNumberParseFloat
//: exit testNumberParseFloat

try {
    test(testParseFloat(obj, 2));
} catch (e) {}
//aot: [trace] aot inline builtin: Number.parseFloat, caller function name:#*#testParseFloat@builtinNumberParseFloat
//aot: [trace] Check Type: NotString1
//: catch 'Error: thr 2' in testParseFloat
//: exit testParseFloat
try {
    test(testNumberParseFloat(obj, 2));
} catch (e) {}
//aot: [trace] aot inline builtin: Number.parseFloat, caller function name:#*#testNumberParseFloat@builtinNumberParseFloat
//aot: [trace] Check Type: NotString1
//: catch 'Error: thr 2' in testNumberParseFloat
//: exit testNumberParseFloat

// Check replace:
Number.parseFloat = () => { throw "CheckReplace" }

test(testParseFloat("1"));

//aot: [trace] aot inline builtin: Number.parseFloat, caller function name:#*#testParseFloat@builtinNumberParseFloat
//: exit testParseFloat
//aot: [trace] aot inline function name: #*#test@builtinNumberParseFloat caller function name: func_main_0@builtinNumberParseFloat
//: number,1

try {
    test(testNumberParseFloat("1"));
} catch (e) {
    if (e != "CheckReplace") throw Error();
}
//aot: [trace] Check Type: NotCallTarget1
//: catch 'CheckReplace' in testNumberParseFloat
//: exit testNumberParseFloat
