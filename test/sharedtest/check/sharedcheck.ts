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

/*
 * @tc.name:sharedcheck
 * @tc.desc:test sharedcheck
 * @tc.type: FUNC
 * @tc.require: issueI8QUU0
 */

// @ts-nocheck
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

/*
 * @tc.name:sharedcheck
 * @tc.desc:test sharedcheck
 * @tc.type: FUNC
 * @tc.require: issueI8QUU0
 */

// @ts-nocheck
declare function print(str: any): string;

class SimpleStringSendable {
  propString: string = "I'm simple sendable's propString";
  constructor() {
    "use sendable"
    print(this.propString)
  }
}

class SimpleNumberSendable {
  propNumber: number = 2023;
  constructor() {
    "use sendable"
    print("I'm constructor for SimpleNumberSendable")
  }
}

class SuperClass {
  propString: string = "I'm propString"
  propNumber: number = 5
  propBool: boolean = false
  static staticPropString: string = "I'm staticPropString";
  #privatePropString: string = "I'm privatePropString";

  get accessorPrivatePropString() {
    return this.#privatePropString;
  }

  set accessorPrivatePropString(prop: string) {
    this.#privatePropString = prop;
  }
  constructor() {
    "use sendable"
    print(this.propString)
  }
}

class SubClass extends SuperClass {
  static staticSubClassPropString: string = "I'm staticSubClassPropString";
  static staticFunc() {
    print("Hi static func");
  }
  static staticFunc1() {
    print("Hi static func1");
  }

  func() {
    print("Hi func");
    this.#privateFunc();
  }

  #privateFunc() {
    print("Hi private func");
  }

  subClassPropString: string = "I'm subClassPropString"
  subClassPropSendable: SimpleStringSendable
  constructor() {
    "use sendable"
    super()
  }
}

function testDelete(testObj: SubClass) {
  print("Start testDelete");
  try {
    delete testObj.propNumber;
    print("Success delete propNumber")
  } catch (error) {
    print("Fail to delete propNumber. err: " + error)
  }
}

function testExtend(testObj: SubClass) {
  print("Start testExtend");
  try {
    Object.defineProperty(testObj, "prop1", { writable: true });
    print("Success extend prop1 with defineProperty")
  } catch (error) {
    print("Fail to extend prop1 with defineProperty. err: " + error)
  }

  try {
    Object.defineProperties(testObj, { prop2: { writable: true } });
    print("Success extend prop2 with defineProperties")
  } catch (error) {
    print("Fail to extend prop2 with defineProperties. err: " + error)
  }

  try {
    testObj.prop3 = {};
    print("Success extend prop3 with stobjbyname")
  } catch (error) {
    print("Fail to extend prop3 with stobjbyname. err: " + error)
  }
}

function testUpdateInstanceFunction(testObj: SubClass) {
  print("Start testUpdateInstanceFunction");
  try {
    testObj.func = function () { }
    print("Success replace instance's func");
  } catch (error) {
    print("Fail to replace instance's func. err: " + error);
  }
}

function testUpdatePrototype(testObj: SubClass) {
  print("Start testUpdatePrototype");
  try {
    SuperClass.prototype.staticSubClassPropString = 1;
    print("Success update prototype")
  } catch (error) {
    print("Fail to update prototype. err: " + error)
  }

  try {
    SubClass.prototype.tmpProp = 123;
    print("Success to extend prop to constructor's prototype.");
  } catch (error) {
    print("Fail to extend prop to constructor's prototype. err: " + error);
  }

  try {
    testObj.__proto__.constructor = a.__proto__.constructor;
    print("Success to change constructor of instance's prototype.");
  } catch (error) {
    print("Fail to change constructor of instance's prototype. err: " + error);
  }

  try {
    testObj.__proto__ = a.__proto__;
    print("Success to replace instance's prototype.");
  } catch (error) {
    print("Fail to replace instance's prototype. err: " + error);
  }

  try {
    Object.defineProperty(testObj.__proto__, "abc", { value: 123 });
    print("Success to extend instance's prototype.");
  } catch (error) {
    print("Fail to extend instance's prototype. err: " + error);
  }
}

function testUpdateInstanceProps(testObj: SubClass) {
  print("Start testUpdateInstanceProps");
  try {
    testObj.propString = null
    print("Success update prop to null with stobjbyname")
  } catch (error) {
    print("Fail to update prop to null with stobjbyname. err: " + error)
  }

  try {
    Object.defineProperties(testObj, { subClassPropString: { value: "hello", writable: true } });
    print("Success update subClassPropString with defineProperties")
  } catch (error) {
    print("Fail to update subClassPropString with defineProperties. err: " + error)
  }

  try {
    Object.defineProperty(testObj, "propNumber", { value: 100, configurable: false });
    print("Success update propNumber with defineProperty")
  } catch (error) {
    print("Fail to update propNumber with defineProperty. err: " + error)
  }

  try {
    testObj.subClassPropString = 33;
    print("Success update subClassPropString with stobjbyname")
  } catch (error) {
    print("Fail to update subClassPropString with stobjbyname. err: " + error)
  }
}

function testUpdateInstanceAccessor(testObj: SubClass) {
  print("Start testUpdateInstanceAccessor");
  try {
    Object.defineProperty(testObj, "accessorPrivatePropString",
      {
        get accessorPrivatePropString() { print("Replaced get accessor") },
        set accessorPrivatePropString(prop: string) { print("Replaced set accessor") }
      })
    print("Success replace accessor");
  } catch (error) {
    print("Fail to replace accessor. err: " + error);
  }

  try {
    testObj.accessorPrivatePropString = "123"
    print("Success set prop through accessor with matched type");
  } catch (error) {
    print("Fail to set prop through accessor with matched type");
  }

  try {
    testObj.accessorPrivatePropString = 123
    print("Success set prop through accessor with mismatched type");
  } catch (error) {
    print("Fail to set prop through accessor with mismatched type");
  }
}

function testUpdateConstructor() {
  print("Start testUpdateConstructor");
  try {
    SubClass.staticFunc = function () { };
    print("Success to modify constructor's method.");
  } catch (error) {
    print("Fail to modify constructor's method. err: " + error);
  }

  try {
    SubClass.staticSubClassPropString = "modify static";
    print("Success to modify property to constructor's property.");
  } catch (error) {
    print("Fail to modify property to constructor's property. err: " + error);
  }
}

function testUpdate(testObj: SubClass) {
  testUpdateInstanceProps(testObj);
  testUpdateInstanceAccessor(testObj);
  testUpdateInstanceFunction(testObj);
  testUpdatePrototype(testObj);
  testUpdateConstructor();
}

function testUpdateWithType(testObj: SubClass) {
  print("Start testUpdateWithType");
  try {
    testObj.propString = 1;
    print("Success update string to int with stobjbynamme.")
  } catch (error) {
    print("Fail to update string to int with stobjbynamme. err: " + error);
  }

  try {
    Object.defineProperty(testObj, "a", { writable: true });
    print("Success update prop with defineProperty")
  } catch (error) {
    print("Fail to define prop with defineProperty. err: " + error);
  }

  try {
    Object.defineProperty(testObj, "subClassPropSendable", { value: 123, writable: true });
    print("Success update subClassPropSendable to number with defineProperty.")
  } catch (error) {
    print("Fail to update subClassPropSendable to number with defineProperty. err: " + error);
  }

  try {
    Object.defineProperty(testObj, "subClassPropSendable", { value: new SimpleNumberSendable(), writable: true });
    print("Success update subClassPropSendable to numberSendable with defineProperty.")
  } catch (error) {
    print("Fail to update subClassPropSendable to numberSendable with defineProperty. err: " + error);
  }
}

let b = new SubClass()
b.subClassPropSendable = new SimpleStringSendable()
testUpdate(b)
testDelete(b)
testExtend(b)
testUpdateWithType(b)