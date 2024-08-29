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
var x;
var mycars = new Array();
mycars[0] = "Saab";
mycars[1] = "Volvo";
mycars[2] = "BMW";
// CHECK#1
var fin = 0;
var i = 0;
for (x in mycars) {
    try {
        i += 1;
        continue;
    }
    catch (er1) { }
    finally {
        fin = 1;
    }
    for (x in mycars) {
        fin = -1;
    }
    fin = -1;
}
print(fin)
print(i)
// CHECK#2
var c2 = 0, fin2 = 0;
for (x in mycars) {
    try {
        throw "ex1";
    }
    catch (er1) {
        c2 += 1;
        continue;
    }
    finally {
        fin2 = 1;
    }
    fin2 = -1;
}
print(fin2)
print(c2)
// CHECK#3
var c3 = 0, fin3 = 0;
for (x in mycars) {
    try {
        throw "ex1";
    }
    catch (er1) {
        c3 += 1;
    }
    finally {
        fin3 = 1;
        continue;
    }
    fin3 = 0;
}
print(fin3)
print(c3)
// CHECK#4
var fin = 0;
for (x in mycars) {
    try {
        continue;
    }
    finally {
        fin = 1;
    }
    fin = -1;
}
print(fin)
// CHECK#5
var c5 = 0;
for (x in mycars) {
    try {
        throw "ex1";
    }
    catch (er1) {
        c5 += 1;
        continue;
    }
    c5 += 12;
}
print(c5)
// CHECK#6
var c6 = 0, fin6 = 0;
for (x in mycars) {
    try {
        c6 += 1;
        throw "ex1";
    }
    finally {
        fin6 = 1;
        continue;
    }
    fin6 = -1;
}
print(fin6)
print(c6)

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
