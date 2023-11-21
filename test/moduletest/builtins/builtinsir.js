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

/*
 * @tc.name:builtins
 * @tc.desc:test builtins
 * @tc.type: FUNC
 * @tc.require: issueI5NO8G
 */
print("builtins ir start");
var str1 = "wpydejkl";
print(str1.charCodeAt(1.231));
print(str1.charCodeAt(undefined));
print(str1.charCodeAt(new Date()));
print(str1.charCodeAt(14));
print(str1.charCodeAt(3));
print(str1.indexOf("wpy"));
print(str1.indexOf("py", 0));
print(str1.indexOf("py", 5));
print(str1.indexOf("hhh", 0));
print(str1.indexOf("hjs", 19));
print(str1.indexOf("jk", 3));
print(str1.indexOf(undefined));
print(str1.indexOf(null));
var person = {
fullName: function(city, country) {
    return this.firstName + " " + this.lastName + "," + city + "," + country;
}
}
var person1 = {
firstName:"Bill",
lastName: "Gates"
}
var a = new Uint32Array([1,2]);
print(person.fullName.apply(person1, a));
// xxxConstructor
var a = {
    test() {
        this.foo();
    }
}
a.foo = Array;
a.test();
a.foo = Boolean;
a.test();
a.foo = Date;
a.test();

var obj = {
    number: Number,
    date: Date,
    bool: Boolean
};
print(obj.number(1));
print(obj.number(1, 2));
print(obj.number(1, 2, 3));
print(obj.number(1, 2, 3, 4));
print(obj.number([1]));
print(obj.number([1, 2]));
obj.date(0);
obj.date(0, 1);
obj.date(0, 1, 2);
obj.date(0, 1, 2, 3);
print(obj.bool(0));
print(obj.bool(0, 1));
print(obj.bool(0, 1, 2));
print(obj.bool(0, 1, 2, 3));
print("builtins ir end");
