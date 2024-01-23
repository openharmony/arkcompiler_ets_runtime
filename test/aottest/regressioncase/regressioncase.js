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

// case 1
function test1() {
    try {
        var base = null;
        var prop = {
            toString: function () {
                print("toString");
                throw "111";
            },
        };
        base[prop];
    } catch (e) {
        print("error");
    }
}

function test2() {
    // case 2
    print(Number.NaN != Number.NaN);
}

// case 3
function _test() {
    this.x = 0.1;
}
function test3() {
    var a = new _test();
    print(a.x);
}

function test4() {
    // case 4: n mod d = r
    // If r = 0 and n < -0, return -0.
    print(1 / (-1 % 1));
    print(1 / (-1 % -1));
    print(1 / (-3 % 1));
    print(1 / (-3 % -1));
    print(1 / (-3 % 3));
    print(1 / (-3 % -3));
    print(1 / (-3.3 % 3.3));
    print(1 / (-3.3 % -3.3));
}

function test5() {
    // case 5: mod
    var a = {};
    a._toString = function (value) {
        if (value === 0 && 1 / value === -Infinity) {
            return "-0";
        }
        return String(value);
    };
    var x;
    x = -1;
    print(a._toString((x %= -1)));
}

function test6() {
    // case6: prototype
    var a = [0];
    a.length = 3;
    Object.prototype[2] = 2;
    print(a[2]);
}

function test7() {
	// onheap mode test
    var onheap = new Uint8Array(1)
    var onheap2 = new Uint8Array(1)
    var notOnHeap = new Uint8Array(512 * 8 + 1)
    onheap[0] = 1  // root on heap
    notOnHeap[0] = 2 // root not onheap-
    onheap2[0] = 3  // root on heap
    print(ArkTools.isOnHeap(onheap));
    print(ArkTools.isOnHeap(notOnHeap));
    print(ArkTools.isOnHeap(onheap2));
    print(onheap[0])
    print(notOnHeap[0])
    onheap.buffer  // root not on heap
    print(ArkTools.isOnHeap(onheap));
    print(onheap[0])
    onheap2.x = 2  // transition
    onheap2.buffer  // clone hclass and set not on heap
    print(ArkTools.isOnHeap(onheap2));
    print(onheap2.x)
    print(onheap2[0])
}

test1()
test2()
test3()
test4()
test5()
test6()
test7()

