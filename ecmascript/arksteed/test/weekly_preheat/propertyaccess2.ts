/*
 * Copyright (c) 2026 Huawei Device Co., Ltd.
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
// import { BenchmarkRunner } from "./benchmarkTsSuite";
// import { ASSERT_NUMBER_EQ, ASSERT_FLOAT_EQ, ASSERT_TRUE, ASSERT_FALSE, ASSERT_EQ } from "../../../utils/assert";
// declare function print(arg:string) : string;

declare interface ArkTools {
  timeInUs(arg:any):number
}

// 多态
class Base {
  baseNum: number = 1
  constructor(num: number){
    this.baseNum = num
  }
  compute(): number {
    return this.baseNum
  }
}

class DeriveDouble extends Base {
  constructor(num: number){
    super(num);
  }
  compute() : number {
    return this.baseNum * 2
  }
}

class DerivedTripple extends Base {
  constructor(num: number){
    super(num);
  }
  compute() : number {
    return this.baseNum * 3
  }
}


function Polymorphism() {
  let count = 100000;
  let result: number[] = new Array();
  let result2: Base[] = new Array();
  let result3: Base[] = new Array();
  for (let i = 0; i < count; i++) {
    result.push(i);
    result2.push(new DeriveDouble(i));
    result3.push(new DerivedTripple(i));
  }
  let start = ArkTools.timeInUs();
  for (let i = 0; i < count; i++) {
    if (result[i] == i) {
      result2[i].baseNum = result2[i].compute();
      result3[i].baseNum = result3[i].compute();
    }
  }
  let end = ArkTools.timeInUs();
  let res:boolean = true
  for (let i = 0; i < count; i++) {
    if (result2[i].baseNum != i * 2 || result3[i].baseNum != i * 3) {
      res = false
    }
  }
  if (!res) {
    print("result is wrong")
  }
  let time = (end - start) / 1000
  print("Property Access - Polymorphism:\t"+String(time)+"\tms");
  return time;
}
// Polymorphism()
// let runner9 = new BenchmarkRunner("Property Access - Polymorphism", Polymorphism);
// runner9.run();

// 单态
class Square{
  length: number = 0;
  width: number = 0;
  constructor(length: number, width: number) {
    this.length = length;
    this.width = width;
  }
}

function GenerateFakeRandomSquare(): Square[] {
  let resource: Square[] = new Array(10);
  for (let i = 0; i < 10; i++) {
    let random1 = Math.random() * (10) + 1;
    let random2 = Math.random() * (10) + 1;
    resource[i] = new Square(random1, random2);
  }
  return resource;
}

function SingleICClass() {
  let container: Square[] = GenerateFakeRandomSquare();
  let count: number = 1000000;
  let arraySize: number = 3;
  let length_res : number[] = [0, 0, 0];
  let width_res : number[] = [0, 0, 0];
  let start = ArkTools.timeInUs();
  for (let i = 0; i < count; i++) {
    for (let j = 0; j < container.length; j++) {
      let thisBox = container[j];
      length_res[i % arraySize] += thisBox.length;
      width_res[i % arraySize] += thisBox.width;
    }
  }
  let end = ArkTools.timeInUs();
  let res = 0;
  for (let j: number = 0; j < arraySize; j++) {
    res += length_res[j] + width_res[j];
  }
  print(""+res);
  let time = (end - start) / 1000
  print("Property Access - SingleICClass:\t"+String(time)+"\tms");
  return time;
}
// SingleICClass()
// let runner2 = new BenchmarkRunner("Property Access - SingleICClass", SingleICClass);
// runner2.run();

// 不可扩展属性
class Person {
  name: string = "";
  age: number = 0;
  constructor(name: string, age: number) {
    this.name = name;
    this.age = age;
  }

  equal(cmp: Person): boolean {
    return this.name == cmp.name && this.age == cmp.age;
  }
}

class Student extends Person {
  university: string = "";
  constructor(name: string, age: number, university: string) {
    super(name, age);
    this.university = university;
  }
}

function GenerateFakeRandomPersons() : Person[] {
  let person1 = new Person("John", Math.random() * (50) + 1);
  let person2 = new Person("John", Math.random() * (50) + 1);
  let person3 = new Person("John", Math.random() * (50) + 1);
  let resources: Person[] = [person1, person2, person3];
  return resources;
}

function NoneExtension() {
  let person1 = new Person("John", 12);
  let person2 = new Person("John", 13);
  let person3 = new Person("John", 14);
  let resourcesPerson: Person[] = [person1, person2, person3];
  let student1: Student = new Student("John", 21, "UNY");
  let count: number = 100000;
  let arraySize: number = 3;
  let res : Int32Array = new Int32Array([0, 0, 0]);
  let test: Person = person1;
  let start = ArkTools.timeInUs();
  for (let i = 0; i < count; i++) {
    // if (resourcesPerson[i % resourcesPerson.length].name == "John" && resourcesPerson[i % resourcesPerson.length].age <= 20) {
    //     if (student1.university == "UNY") {
    //         res[i % arraySize] += 1;
    //     }
    // } else {
    //     if (student1.age == 21) {
    //         res[i % arraySize] += 2;
    //     }
    // }
    let a = resourcesPerson[i % arraySize];
    if (a.equal(person1)) {
      let b = resourcesPerson[(i + 1) % arraySize];
      if (b.equal(person2)) {
        let c = resourcesPerson[(i + 2) % arraySize];
        if (c.equal(person3)) {
          test = c;
        }
      }
    }
    if (i % arraySize == 0 && test.equal(person3)) {
      res[i % 3] += 1
      test = person1;
    }
    if (i % arraySize == 0 && test.equal(person1)) {
      res[i % 3] += 1
      test = person2;
    }
  }
  let end = ArkTools.timeInUs();
  let sum = 0;
  for (let j: number = 0; j < arraySize; j++) {
    sum += res[j]
  }
  print(""+sum);
  let time = (end - start) / 1000
  print("Property Access - NoneExtension:\t"+String(time)+"\tms");
  return time;
}
// NoneExtension()
// let runner3 = new BenchmarkRunner("Property Access - NoneExtension", NoneExtension);
// runner3.run();

// Getter & Setter
class MyObject {
  num: number;
  age: number;
  constructor(num: number, age: number) {
    this.num = num;
    this.age = age;
  }
  get Getter() {
    return this.num;
  }
  set Setter(n: number) {
    this.age = n;
  }
}

function GenerateNumArray(count: number): number[] {
  let numArray: number[] = [0, 0, 0, 0, 0, 0, 0, 0, 0, 0];
  for (let i = 0; i < count; i++) {
    numArray[i] = i;
  }
  return numArray;
}

function GenerateResultArray(count: number): MyObject[] {
  let result = [{}, {}, {}, {}, {}, {}, {}, {}, {}, {}];
  for (let i = 0; i < count; i++) {
    result[i] = new MyObject(i, i);
  }
  return result;
}

function GetterSetterTest() {
  let count: number = 10000000;
  let result: MyObject[] = GenerateResultArray(10);
  let numArray: number[] = GenerateNumArray(10);

  let start = ArkTools.timeInUs();
  let resLen: number = result.length;
  for (let i = 0; i < count; i++) {
    let index = i % resLen;
    let ret = result[index];
    if (ret.Getter == numArray[index]) {
      ret.Setter = index;
    }
  }
  let end = ArkTools.timeInUs();
  let time = (end - start) / 1000
  print("Property Access - GetterSetterTest:\t"+String(time)+"\tms");
  return result;
}

function GetterSetterRes() {
  let result = GetterSetterTest();
  let numArray = GenerateNumArray(result.length);
  let sum = 0;
  let res = true;
  for (let i = 0; i < result.length; i++) {
    sum += result[i].num
    if (result[i].age != numArray[i]) {
      res = false;
      break;
    }
  }
  print(""+res);
  print(""+sum);
}
// GetterSetterRes()

// let runner5 = new BenchmarkRunner("Property Access - GetterSetterTest", GetterSetterTest);
// runner5.run();


// All Number object
class Grades {
  math: number = 0.0
  english: number = 0.0
  physics: number = 0.0
  chemistry: number = 0.0
  constructor(math: number, english: number, physics: number, chemistry: number) {
    this.math = math;
    this.english = english;
    this.physics = physics;
    this.chemistry = chemistry;
  }
}

function GenerateFakeRandomGrades(): Grades[] {
  let resource: Grades[] = new Array(10);
  for (let i = 0; i < 10; i++) {
    let random1 = Math.random() * (100.0) + 1.0;
    let random2 = Math.random() * (100.0) + 1.0;
    let random3 = Math.random() * (100.0) + 1.0;
    let random4 = Math.random() * (100.0) + 1.0;
    resource[i] = new Grades(random1, random2, random3, random4);
  }
  return resource;
}

function NumberObject() {
  let count: number = 1000000;
  let arraySize: number = 2;
  let myGrades: Grades[] = GenerateFakeRandomGrades();
  let res: Float64Array = new Float64Array([0, 0, 0]);
  let start = ArkTools.timeInUs();
  let gradesLength = myGrades.length - 1;
  for (let i = 0; i < count; i++) {
    let ret = res[i & arraySize];
    let grades = myGrades[i & gradesLength];
    ret += grades.chemistry;
    ret += grades.english;
    ret += grades.math;
    ret += grades.physics;
    res[i & arraySize] = ret;
  }
  let end = ArkTools.timeInUs();
  let sum = 0;
  for (let j = 0; j < arraySize; j++) {
    sum += res[j];
  }
  print(""+sum);
  let time = (end - start) / 1000
  print("Property Access - NumberObject:\t"+String(time)+"\tms");
  return time;
}
// NumberObject()
// let runner7 = new BenchmarkRunner("Property Access - NumberObject", NumberObject);
// runner7.run();




let loopCountForPreheat = 1;

for (let i = 0; i < loopCountForPreheat; i++) {
  Polymorphism()
  SingleICClass()
  NoneExtension()
  GetterSetterRes()
  NumberObject()
}

function sleep(delay) {
    const start = Date.now();
    while (Date.now() - start < delay) {}
}

sleep(5000);

Polymorphism()
SingleICClass()
NoneExtension()
GetterSetterRes()
NumberObject()
