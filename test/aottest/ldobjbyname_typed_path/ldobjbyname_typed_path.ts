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

declare function print(...args: any[]): void;

// Test Array.prototype methods
{
    // May be slow path (since the type of arrayLiteral is not marked explicitly)
    print("Testing Array literal:");
    let arrayLiteral = [7, 8, 9];
    print("  - get length: ", arrayLiteral.length);
    print("  - Array.prototype.reverse:     ", arrayLiteral.reverse()); // arr = [9, 8, 7]
    print("  - Array.prototype.shift:       ", arrayLiteral.shift()); // return: 9, arr = [8, 7]

    // Expects AOT typed path.
    print("Testing new Array():");
    let newArrayNumber = new Array(10, 11, 12, 13, 14, 15);
    print("  - get length: ", newArrayNumber.length);
    print("  - Array.prototype.slice:       ", newArrayNumber.slice(1, 4)); // return: [11, 12, 13]
    print("  - Array.prototype.includes:    ", newArrayNumber.includes(12)); // return: true
}
// Test DataView.prototype.methods
// Expects AOT typed path
{
    print("Testing DataView:");
    let data = new ArrayBuffer(16);
    let dataView = new DataView(data);
    print("  - DataView.prototype.getInt32:   ", dataView.getInt32(0)); // return: 0
    print("  - DataView.prototype.getFloat32: ", dataView.getBigInt64(8)); // return: 0
}
// Test Date.prototype methods
// Expects AOT typed path.
{
    print("Testing Date:");
    let date = new Date("Sep 26 2023 9:45:30 GMT+0800"); // 2023/9/26 9:45:30
    print("  - Date.prototype.setFullYear:  ", date.setFullYear(2022)); // return: 1664156730000
    print("  - Date.prototype.toDateString: ", date.toDateString()); // return: 'Mon Sep 26 2022'
}
// Test Map.prototype methods
// Expects AOT typed path.
{
    print("Testing Map:");
    let map = new Map([
        ['a', 1],
        ['b', 2],
        ['c', 3]
    ]);
    print("  - get size: ", map.size);
    print("  - Map.prototype.has: ", map.has('A')); // return: false
    print("  - Map.prototype.get: ", map.get('a')); // return: 1
}
// Test Set.prototype methods
// Expects AOT typed path.
{
    print("Testing Set:");
    let set = new Set([1, 2, 3]);
    print("  - get size: ", set.size); // return: 3
    print("  - Set.prototype.has: ", set.has(4)); // return: false
}
// Test String.prototype.methods
{
    // Expects AOT typed path.
    print("Testing string literal:");
    let strLiteral = "I am a string literal";
    print("  - get length: ", strLiteral.length);
    print("  - String.prototype.charAt:     ", strLiteral.charAt(2)); // return: 'a'
    print("  - String.prototype.substring:  ", strLiteral.substring(7)); // return: 'string literal'

    // May be slow path (since JS_PRIMITIVE_REF is not included in type paths currently)
    print("Testing new String():");
    let newString = new String("I am a String instance");
    print("  - get length: ", newString.length);
    print("  - String.prototype.indexOf:    ", newString.indexOf("String")); // return: 7
    print("  - String.prototype.startsWith: ", newString.startsWith("I am a")); // return: true
}
// Test %TypedArray%.prototype methods
// Expects AOT typed path
{
    print("Testing Int16Array:");
    let i16Array = new Int16Array([1, 2, 3]);
    print("  - get length: ", i16Array.length);
    print("  - Int16Array.prototype.indexOf: ", i16Array.indexOf(3)); // return: 2
    print("  - Int16Array.prototype.join:    ", i16Array.join(' - ')); // return: '1 - 2 - 3'

    print("Testing BigUint64Array:");
    let u64Array = new BigUint64Array([4n, 5n, 6n]);
    print("  - get length: ", u64Array.length);
    print("  - BigUint64Array.prototype.includes:    ", u64Array.includes(7n)); // return: false
    print("  - BigUint64Array.prototype.lastIndexOf: ", u64Array.lastIndexOf(4n)); // return: 0
}
// Test object literal with prototype changes.
{
    print("Testing object literal with prototype changed (via obj.__proto__):");
    let obj = {
        substring: function() {
            return "obj.substring";
        }
    };
    obj.__proto__ = String.prototype;
    print("  - obj.substring: ", obj.substring()); // return: 'obj.substring'
}
// Test instance of type A whose prototype is changed to B.prototype
{
    print("Testing Array with prototype changed (via arr.__proto__):");
    let arr: Array<number> = [7, 8, 9];
    arr.substring = function() {
        return "arr.substring";
    };
    arr.__proto__ = String.prototype;
    print("  - arr.substring: ", arr.substring()); // return: 'arr.substring'
}
// Test modifying properties Array.prototype
// Expects AOT Typed path (if array stability is kept) since HClass of Array.prototype is not changed
Array.prototype.pop = function (): any {
    return "Array.prototype.pop changed to another function.";
};
{
    print("Testing with Array.prototype.pop changed to another function:");
    let arr: Array<number> = [1, 2, 3];
    print("  - Array.prototype.pop: ", arr.pop());
}
// Expects AOT Typed path (if array stability is kept) since HClass of Array.prototype is still not changed
Object.defineProperty(Array.prototype, 'pop', { value: "Array.prototype.pop changed to a string." });
{
    print("Testing with Array.prototype.pop changed to a string:");
    let arr: Array<number> = [1, 2, 3];
    print("  - Array.prototype.pop: ", arr.pop);
}
// Expects de-opt due to HClass of Array.prototype mismatch
Object.defineProperty(Array.prototype, 'pop', {
    get() { return "Array.prototype.pop changed to a getter."; }
});
{
    print("Testing with Array.prototype.pop changed to a getter:");
    let arr: Array<number> = [1, 2, 3];
    print("  - Array.prototype.pop: ", arr.pop);
}
