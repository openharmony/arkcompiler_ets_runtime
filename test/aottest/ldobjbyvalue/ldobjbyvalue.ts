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

declare function print(arg:any):string;

class A{}

function testLiteralArray() {
    let array:A[] = [undefined];
    for (let i = 0; i < array.length; i++) {
        print(array[i]===undefined);
    }
}

testLiteralArray();

function testLargeHolyArray() {
    let arrayTest = new Array(10000);
    print(arrayTest[9] === undefined);
}

testLargeHolyArray();

function testHolyArray() {
    let arrayTest = new Array(10);
    print(arrayTest[9] === undefined);
}
testHolyArray();

function testStringIndex() {
    class A {
        constructor() {
            var gotoFn = {
                0: {}
            };
        
            this.gotoFn = gotoFn;
        }
        
        search(string) {
            var state = 0;
            var results = [];
            for (var i=0; i<string.length; i++) {
                var l = string[i];
                if (!(l in this.gotoFn[state])) {
                    continue;
                }
                state = this.gotoFn[state][l];
            }
        
            return results;
        };
    }


    let words = ""
    for (let i = 0; i < 100; i++) {
        words += "should find keyword" + i + " at position " + i + "and"
    }
    let ac = new A();
    for (let i = 0; i < 100; i++) {
        ac.search(words)
    }
}
testStringIndex();
