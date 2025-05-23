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
const animals = ["Panda", "Tiger", "Bird"];

//check_1
let finalValue = 0;
let count = 0;
for (const animal of animals) {
    try {
        count++;
        continue;
    } catch (error) {}
    finally {
        finalValue = 1;
    }
    for (const animal of animals) {
        finalValue = -1;
    }
    finalValue = -1;
}
print(finalValue);
print(count);

//check_2
let finalValue2 = 0;
let count2 = 0;
for (const animal of animals) {
    try {
        throw "exception1";
    } catch (error) {
        count2++;
        continue;
    } finally {
        finalValue2 = 1;
    }
    finalValue2 = -1;
}
print(finalValue2);
print(count2);

//check_3
let finalValue3 = 0;
let count3 = 0;
for (const animal of animals) {
    try {
        throw "exception1";
    } catch (error) {
        count3++;
    } finally {
        finalValue3 = 1;
        continue;
    }
    finalValue3 = 0;
}
print(finalValue3);
print(count3);

//check_4
let finalValue4 = 0;
for (const animal of animals) {
    try {
        continue;
    } finally {
        finalValue4 = 1;
    }
    finalValue4 = -1;
}
print(finalValue4);

//check_5
let count5 = 0;
for (const animal of animals) {
    try {
        throw "exception1";
    } catch (error) {
        count5++;
        continue;
    }
    count5 +=12;
}
print(count5);

//check_6
let finalValue6 = 0;
let count6 = 0;
for (const animal of animals) {
    try {
        count6++;
        throw "exception1";
    } finally {
        finalValue6 = 1;
        continue;
    }
    finalValue6 = -1;
}
print(finalValue6);
print(count6);

class A {
    constructor(a:any|number) {
        try {
            a = -1234.5678;
            function f():void {}
            f(f, a);
        } finally {
            Symbol[a] = 1.0
        }
    }
}

let o1 = {}
for (let i = 0 ; i < 1; i++) {
    try {
        continue;
    } catch(e19) {
        [...o1] = [0]
    } finally {
        try {
            o1[Math]
        } catch (e) {
        }
    }
}

let v = []
for (const vv in v) {
    try {
        continue;
    } catch(e19) {
        [...o1] = [0]
    } finally {
        try {
            o1[Math]
        } catch (e) {
        }
    }
}

var c=0, i=0;
var fin=0;
while(i<10){
  i+=1;
  try{
    if(c===0){
      throw "ex1";
    }
    c+=2;
    if(c===1){
      throw "ex2";
    }
  }
  catch(er1){
    c-=1;
    continue;
  }
  finally{
    fin+=1;
  }
}
print(fin)
