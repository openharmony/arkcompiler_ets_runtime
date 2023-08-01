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
declare function print(arg:any):void;

function testcase1() {
    let testArray: Int32Array = new Int32Array([1, 2, 3]);
    let len: number = testArray.length;
    let a: number = testArray[0];
    let res: number = 5 + len;
    print(res)
}

function testcase2() {
    let testArray: Int32Array = new Int32Array([1, 2, 3]);
    let a: number = testArray[0];
    let len: number = testArray.length - 1;
    let res: number = 6 + len;
    print(res)
}

function testcase3() {
    let testArray: Int32Array = new Int32Array([1, 2, 3]);
    let len: number = testArray.length;
    let sum: number = 0;
    for(let i = 0; i < len; i++) {
        print(i + 1);
        sum += testArray[i];
    }
}

function testcase4() {
    let testArray: Int32Array = new Int32Array([1, 2, 3]);
    let len: number = testArray.length - 1;
    let sum: number = 0;
    for(let i = 0; i < len; i++) {
        sum += testArray[i];
        print(i + 1);
    }
}

function testcase5() {
    let testArray: number[] = [1, 2, 3];
    let a: number = testArray[0];
    let res: number = 5 + testArray.length;
    print(res);
}

function testcase6() {
    let testArray: number[] = [1, 2, 3];
    let len: number = testArray.length - 1;
    let sum: number = 0;
    let i: number = 0;
    for(let i = 0; i < len; i++) {
        sum += testArray[i];
        print(i + 1);
    }
}

//mod CheckRightIsZero
function testcase7() {
    const a: number= 20;
    let b: number = 5 % a;// not gen CheckRightIsZero
    let c: number = 5 % b;// gen CheckRightIsZero
}

//mul_with_overcheck
function testcase8(a:number, x:number) {
    let z = a >>> 1;
    let b: number = z % 10;//range[0, 4095]

    let c: number = 10 * b;//not gen mul_with_overcheck, range[0,262143]
    let d: number = c * b;//not gen mul_with_overcheck, range[0,1073741823]
    let e: number = d * c;//gen mul_with_overcheck

    x = x >>> 1;
    let h: number = x * x;//gen mul_with_overcheck
}

testcase1();  // 8
testcase2();  // 8

testcase3();
testcase4();

testcase5(); 
testcase6();

testcase7();
testcase8(4, 107341001);//deopt