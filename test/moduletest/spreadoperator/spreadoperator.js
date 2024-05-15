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
print(arr1.length);
print(arr1);

var arr2 = [1, 2, 4, 6, 7, 8, 9, 54];
var arr3 = [...arr2];
print(arr3.length);
print(arr3);

// test spread Set
const set1 = new Set();
set1.add(42);
set1.add(42);
set1.add(13);
set1.add(23);

var arr4 = [...set1.keys()];
print(arr4.length);
print(arr4);

var arr5 = [...set1];
print(arr5.length);
print(arr5);

// test spread map
const map1 = new Map();
map1.set('a', 1);
map1.set('b', 2);
map1.set('c', 3);

var arr6 = [...map1.keys()];
print(arr6.length);
print(arr6);

var arr7 = [...map1.values()];
print(arr7.length);
print(arr7);

// test change Symbol.iterator
let iterFunc = function *() {
    yield 1;
    yield 2;
    yield 3;
}
print(...map1);
Map.prototype[Symbol.iterator] = iterFunc;
print(...map1);

let set  = new Set()
set.add('a');
set.add('b');
set.add('c');
print(...set);
Set.prototype[Symbol.iterator] = iterFunc;
print(...set);

let uint8 = new Uint8Array(2);
print(...uint8);
Uint8Array.prototype[Symbol.iterator] = iterFunc;
print(...uint8);

let arr8 = ['foo'];
let warn1 = print.bind(print);
function show1(message, ...args) {
    return warn1(message, ...args);
}
show1(...arr8);

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
proxy(...arr9);

let fruits1 = ['Apple']
let fruits2 = ['Apple', 'Banana']
print(...fruits2)
Array.prototype[Symbol.iterator] = function* () {
    yield 1;
    yield 2;
    yield 3;
}
print(...fruits1)
print(...fruits2)

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
for (let i = 0; i < 5; ++i) { 
    print(app.childNodes[i].childNodes[0].childNodes[0].content); 
}
let result = []
try {
    class C29 {};
    const v66 = undefined;
    new C29(...v66);
} catch (err) {
    result.push(err.name == "TypeError");
}
print(result)

let arr=[];
print([,...arr].length)
print([,...''].length)
