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
 * @tc.name:generator
 * @tc.desc:test generator
 * @tc.type: FUNC
 * @tc.require: issueI5NO8G
 */
function* gen(){
    yield 1;
    yield 2;
}

var g = gen();
g[Symbol.iterator] = function*(){
    yield 1;
    yield 2;
    yield 3;
};
for (var i of g){
    print(i);//1,2,3
}

function *foo() {
    yield 1;
    yield 2;
}

var p = foo()
var a = p.next()
print(a.value, a.done)
var b = p.next()
print(b.value, b.done)
var c = p.next()
print(c.value, c.done)
var d = p.next()
print(d.value, d.done)

function* g1() { yield 1; }
var iter = g1();
var gf = iter.__proto__.constructor;
print(Object.prototype.toString.call(gf));

var GeneratorFunctionPrototype = Object.getPrototypeOf(g1);
var GeneratorFunction = GeneratorFunctionPrototype.constructor;
var GeneratorObjectPrototype = GeneratorFunctionPrototype.prototype;
print(Object.getPrototypeOf(GeneratorFunction));
var found_property_names = Object.getOwnPropertyNames(GeneratorObjectPrototype);
found_property_names.sort();
print(found_property_names);