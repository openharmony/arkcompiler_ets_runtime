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

function newNumber(a : any, shouldThrow = 0): any
{
    try {
        if (shouldThrow == 1) {
            throw Error("thr 1");
        }
        let x = new Number(a);
        if (shouldThrow == 2) {
            throw Error("thr 2");
        }
        return x;
    } catch (e) {
        print("catch", "'" + e + "'", "in newNumber");
        throw (e)
    } finally {
        print("exit newNumber");
    }
}
function newNumberZero(): any
{
    return new Number()
}

function test(a : any)
{
    a.neg = -a;
    let checks = [typeof a, a instanceof Number, a.constructor.name, 0 + a, a.neg];
    print(checks)
}

test(newNumber(12));
//: exit newNumber
//aot: [trace] aot inline function name: #*#test@builtinNumberConstructor caller function name: func_main_0@builtinNumberConstructor
//: object,true,Number,12,-12

test(newNumber("12"));
//: exit newNumber
//aot: [trace] aot inline function name: #*#test@builtinNumberConstructor caller function name: func_main_0@builtinNumberConstructor
//: object,true,Number,12,-12

//aot: [trace] Check Type: NotInt1
test(newNumber("1.2"));  // Only compressed strings are supported
//: exit newNumber
//aot: [trace] aot inline function name: #*#test@builtinNumberConstructor caller function name: func_main_0@builtinNumberConstructor
//: object,true,Number,1.2,-1.2

test(newNumber(1.2));
//: exit newNumber
//aot: [trace] aot inline function name: #*#test@builtinNumberConstructor caller function name: func_main_0@builtinNumberConstructor
//: object,true,Number,1.2,-1.2

//aot: [trace] aot inline function name: #*#newNumberZero@builtinNumberConstructor caller function name: func_main_0@builtinNumberConstructor
//aot: [trace] aot inline function name: #*#test@builtinNumberConstructor caller function name: func_main_0@builtinNumberConstructor
test(newNumberZero());
//: object,true,Number,0,0

let numberFunction = Number;
Number = String;

//aot: [trace] Check Type: NewBuiltinCtorFail1
test(newNumber(12));
//: exit newNumber
//: object,true,String,012,-12

//aot: [trace] Check Type: NewBuiltinCtorFail1
let tmp = newNumber(12);
//: exit newNumber
Number = numberFunction;
test(tmp);
//: object,false,String,012,-12

//aot: [trace] Check Type: NotString1
test(newNumber(12n));
//: exit newNumber
//: object,true,Number,12,-12

class Foo {};
//aot: [trace] Check Type: NotString1
test(newNumber(new Foo()));
//: exit newNumber
//: object,true,Number,NaN,NaN

//aot: [trace] Check Type: NotString1
test(newNumber(new Object()));
//: exit newNumber
//: object,true,Number,NaN,NaN

try {
    newNumber(7, 1);
    //: catch 'Error: thr 1' in newNumber
    //: exit newNumber
} catch (e) {
    print("catch", "'" + e + "'", "in main");
    //: catch 'Error: thr 1' in main
}

try {
    newNumber(7, 2);
    //: catch 'Error: thr 2' in newNumber
    //: exit newNumber
} catch (e) {
    print("catch", "'" + e + "'", "in main");
    //: catch 'Error: thr 2' in main
}
