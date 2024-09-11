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

function createArray(len) {
    return Array.apply(null, { length: len });
}

let arr = createArray(10);
let a1 = createArray(23.2);
let a2 = createArray();
print(arr);
print(a1);
print(a2.length);
const v1 = 'ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789';
const v4 = Int8Array.from(v1, v5 => v5.charCodeAt(0));
Object.defineProperty(v4, "length", {
    value: 0
});
print(String.fromCharCode.apply(null, v4));

function f0(a, b) {
    print(a, b);
}

let v38;

function f2() {
    arguments.length = -1;
    v38 = arguments;
}

f2(1, 2);
f0.apply(null, v38);

// undefined
try {
    const v3 = new ArrayBuffer(17);

    function F4(a6, a7) {
        if (!new.target) {
            throw 'must be called with new';
        }

        function f8(a9, a10, a11) {
        }

        const v14 = new BigUint64Array(31);
        const o15 = {
            ...v14,
            ...v14,
        };
        Object.defineProperty(o15, 4, { set: f8 });
    }

    const v16 = new F4();
    F4(v16, v3);
} catch (error) {
    print(error.name)
}

// undefined
try {
    const v3 = [100, Int8Array];
    const v6 = new ArrayBuffer(13);

    function F7(a9, a10) {
        if (!new.target) {
            throw 'must be called with new';
        }

        function f11(a12, a13, a14) {
        }

        const v17 = new BigUint64Array(35);
        const o18 = {
            ...v17,
            ...v17,
        };
        Object.defineProperty(o18, 4, { set: f11 });
    }

    const v19 = new F7();
    F7(v19, v6);
    JSON.stringify(6, v3);
} catch (error) {
    print(error.name)
}

print(Function.name);
print(Function.length);
print(Function.prototype.name);
print(Function.prototype.length);

for (let key in Function) {
    print(key);
}

for (let key in Function.prototype) {
    print(key);
}

var backup_apply = Function.prototype.apply;
Function.prototype.constructor = 1;
Function.prototype.apply = 1;
Function.prototype.bind = 1;
Function.prototype.call = 1;
Function.prototype.toString = 1;

try{
   Function.name = 1;
} catch(e) {
   print(e);
}

try{
   Function.length = 1;
} catch(e) {
   print(e);
}

try{
   Function.prototype = 1;
} catch(e) {
   print(e);
}

try{
   Function.prototype.name = 1;
} catch(e) {
   print(e);
}

try{
   Function.prototype.length = 1;
} catch(e) {
   print(e);
}

try{
   Function.prototype.caller = 1;
} catch(e) {
   print(e);
}

try{
   Function.prototype.arguments = 1;
} catch(e) {
   print(e);
}


try{
   Function.prototype[Symbol.hasInstance] = 1;
} catch(e) {
   print(e);
}

Function.prototype.apply = backup_apply;

let a3 = [1, 2, 3];
function func1(a, b, c, d) {
    arguments.length = 100;
    func2.apply(null, arguments, 0, 0); // c interpreter
}
function func2(a) {
    print(a.length);
    for (let i = 0; i < a.length; ++i) {
        print(a[i]);
    }
}
func1(a3);
