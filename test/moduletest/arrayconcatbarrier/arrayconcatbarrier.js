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
 * Regression test for the marking write-barrier on huge arrays.
 *
 * Array.prototype.concat copies its second source into the result at an offset past the
 * first source. When the result is a huge object, that copy destination can sit more than
 * one GC region (256KB) past the array header, so the marking barrier must derive the owning
 * Region from the header rather than from the interior copy address. The latter resolves a
 * bogus Region and crashes in Barriers::Update while concurrent marking is active.
 *
 * Array_9 (length 59321) is a huge object; Array_34 holds a heap object whose copy lands past
 * a region boundary. The large string / ArrayBuffer allocations plus retaining the concat
 * results keep concurrent full marking active across the loop. The test passes if it runs to
 * completion without crashing.
 */
var String_1 = 'a'.repeat(45760);
var ArrayBuffer_5 = new ArrayBuffer(61328, {maxByteLength:402128});
var Array_8 = new Array(1074);
var Array_9 = new Array(450);
var Array_12 = Array_8.reverse();           // heap object
Array_9.length = 41283;
var String_14 = String_1.replace(/[a-zA-Z]/gi, "aaa");
Array_9.length = 59321;
Array_9.push(Array_12);
var Array_34 = Array_9.toReversed();         // Array_34[0] is the heap object Array_12
Array_9.fill(1337);                          // Array_9 = smis; Array_34 still holds the heap object

var keep = [];
var WINDOW = 200;                            // ~200 * 950KB ~= 190MB live -> keeps full marking active
var result;
for (var i = 0; i < 3000; i++) {
    try { ArrayBuffer_5.slice(); } catch (e) {}
    result = Array_9.concat(Array_34);       // huge cross-region copy under concurrent marking
    keep.push(result);
    if (keep.length > WINDOW) {
        keep.shift();
    }
}

assert_equal(result.length, 59322 + 59322);
assert_true(result[0] === 1337);
assert_true(result[59322] === Array_12);     // tail copied correctly across the region boundary

test_end();
