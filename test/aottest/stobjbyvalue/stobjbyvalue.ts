/*
 * Copyright (c) 2022 Huawei Device Co., Ltd.
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

declare function print(str:any):string;

var first = 0;
var second = 1;
var hundred = "100";

// test array
var array = [100, "hello"];
print(array[first]);
print(array[second]);
array[first] = "helloworld";
array[second] = 200;
print(array[first]);
print(array[second]);

let arr: number[] = [1, 2, 3, 4, 5, 6, 7, 8, 9, 10];
arr[0] = 999;
print(arr[0])


// test object
let phrase: { 0: string, "100": string | number, fullPhrase: any } = {
    0 : "100",
    "100" : "hello",

    get fullPhrase() {
        return `${this[first]} ${this[hundred]}`;
    },

    set fullPhrase(value) {
        [this[first], this[hundred]] = value.split(" ");
    }
};
print(phrase[first]);
print(phrase[hundred]);
phrase[first] = "helloworld";
phrase[hundred] = 1;
print(phrase[first]);
print(phrase[hundred]);

// test class
class ParticleSystemCPU {
    private _arrayTest: number[];

    constructor() {
        this._arrayTest = new Array(10);
        this._arrayTest[9] = 19;
        print(this._arrayTest[9]); // 19
    }
}

var system: ParticleSystemCPU[] = [new ParticleSystemCPU()];


// test getter and setter
print(phrase.fullPhrase);
phrase.fullPhrase = "world hello";
print(phrase.fullPhrase);

// test COW array
function generateCOWArray() : number[] {
    return [0, 1, 2, 3, 4]
}

let arrayA : number [] = generateCOWArray();
let arrayB : number [] = generateCOWArray();

arrayB[3] = 11;
print(arrayB[3]) // expect 11
print(arrayA[3]) // expect 3