/*
 * Copyright (c) 2021 Huawei Device Co., Ltd.
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
 * @tc.name:spreadoperator
 * @tc.desc:test spread (...)
 * @tc.type: FUNC
 * @tc.require: issueI5NO8G
 */
// test spread Array
var arr1 = [...Array(16).keys()];
assert_equal(arr1.length,16);
assert_equal(arr1,[0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15]);

var arr2 = [1, 2, 4, 6, 7, 8, 9, 54];
var arr3 = [...arr2];
assert_equal(arr3.length,8);
assert_equal(arr3,[1, 2, 4, 6, 7, 8, 9, 54]);

// test spread Set
const set1 = new Set();
set1.add(42);
set1.add(42);
set1.add(13);
set1.add(23);

var arr4 = [...set1.keys()];
assert_equal(arr4.length,3);
assert_equal(arr4,[42, 13, 23]);

var arr5 = [...set1];
assert_equal(arr5.length,3);
assert_equal(arr5,[42, 13, 23]);

// test spread map
const map1 = new Map();
map1.set('a', 1);
map1.set('b', 2);
map1.set('c', 3);

var arr6 = [...map1.keys()];
assert_equal(arr6.length,3);
assert_equal(arr6,["a", "b", "c"]);

var arr7 = [...map1.values()];
assert_equal(arr7.length,3);
assert_equal(arr7,[1, 2, 3]);

// test change Symbol.iterator
let iterFunc = function *() {
    yield 1;
    yield 2;
    yield 3;
}
let expandedMap = [...map1];
assert_equal(JSON.stringify(expandedMap),JSON.stringify([["a", 1], ["b", 2], ["c", 3]]));
Map.prototype[Symbol.iterator] = iterFunc;
let objkey1 = [...map1];
assert_equal(objkey1,[1, 2, 3]);

let set  = new Set()
set.add('a');
set.add('b');
set.add('c');
let objkey2 = [...set];
assert_equal(objkey2,["a", "b", "c"]);
Set.prototype[Symbol.iterator] = iterFunc;
let objkey3 = [...set];
assert_equal(objkey3,[1, 2, 3]);

let uint8 = new Uint8Array(2);
let objkey4 = [...uint8];
assert_equal(objkey4,[0, 0]);
Uint8Array.prototype[Symbol.iterator] = iterFunc;
let objkey5 = [...uint8];
assert_equal(objkey5,[1, 2, 3]);

let arr8 = ['foo'];
let warn1 = print.bind(print);
function show1(message, ...args) {
    return warn1(message, ...args);
}
assert_equal(...arr8,'foo');

let arr9 = ['foo'];
let warn2 = print.bind(print);
function show2(message, ...args) {
    warn2(message, ...args);
}
const handler = {
    apply: function (target, thisArg, argumentsList) {
        return target(...argumentsList);
    }
};
let proxy = new Proxy(show2, handler);
assert_equal(...arr9,"foo");

let fruits1 = ['Apple']
let fruits2 = ['Apple', 'Banana']
let objkey6 = [...fruits2];
assert_equal(objkey6,["Apple", "Banana"]);
Array.prototype[Symbol.iterator] = function* () {
    yield 1;
    yield 2;
    yield 3;
}
let objkey7 = [...fruits1];
assert_equal(objkey7,['Apple']);
let objkey8 = [...fruits2];
assert_equal(objkey8,['Apple', 'Banana']);

// test spread array when encounter situations like [...arr, elem1, elem2] with arr be StableJSArray 
function appendChild(newNode) { 
    this.childNodes = [...this.childNodes, newNode];
}
const app = { tageName: 'VIEW', childNodes: [], appendChild };
for (let i = 0; i < 5; ++i) { 
    const el = { tageName: 'VIEW', childNodes: [], appendChild };
    const text = { tageName: 'VIEW', childNodes: [], appendChild };
    const content = { tageName: '#text', content: i }; 
    text.appendChild(content); 
    el.appendChild(text); 
    app.appendChild(el);
}
let objkey9 = [];
for (let i = 0; i < 5; ++i) { 
    objkey9.push(app.childNodes[i].childNodes[0].childNodes[0].content); 
}
assert_equal(objkey9,[0, 1, 2, 3, 4]);

let result = []
try {
    class C29 {};
    const v66 = undefined;
    new C29(...v66);
} catch (err) {
    result.push(err.name == "TypeError");
}
assert_equal(result,[true]);

let arr=[];
assert_equal([,...arr].length,1);
assert_equal([,...''].length,1);

test_end();