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

class P {
    constructor() {
        this.a = 1;
    }
}
// test normal object
let proto = new P();
let protoT = new P();

// use to store results
let conditionArray = [];
conditionArray.push(!(proto === protoT))
// share hclass
conditionArray.push(ArkTools.compareHClass(proto, protoT));
conditionArray.push(!ArkTools.isPrototype(proto));
conditionArray.push(!ArkTools.isPrototype(protoT));

print("start test!!!")
let obj1 = {"b": "2b"};
let obj2 = {"c": "3c"};
let obj3 = {"d": "4d"};

let obj11 = {"bb": "2bb"};
let obj22 = {"cc": "3cc"};
let obj33 = {"dd": "4dd"};

Object.setPrototypeOf(obj1, proto);
Object.setPrototypeOf(obj2, proto);
Object.setPrototypeOf(obj3, proto);

conditionArray.push(ArkTools.isPrototype(proto));
conditionArray.push(!ArkTools.isPrototype(protoT));
conditionArray.push(!ArkTools.compareHClass(proto, protoT));

// set proto of obj11, obj22, obj33
// obj11 --> proto --> obj1 --> proto --> proto object
// obj22 --> proto --> obj1 --> proto --> proto object
// obj33 --> proto --> obj2 --> proto --> proto object
obj11.__proto__ = obj1;
obj22.__proto__ = obj1;
obj33.__proto__ = obj2;

function test() {
    for (let i = 0; i <= 50; i++) {
        obj11["a"];
    }
}
test();
protoT["x"] = "2000x";
obj1["www"] = "100www";

// test proxy
let protoP1 = new P();
let protoP2 = new P();
protoP1.a = "xxx";

let proxyOfProto = new Proxy(protoP1, {});
conditionArray.push(!(protoP1 === protoP2))
conditionArray.push(ArkTools.compareHClass(protoP1, protoP2));
conditionArray.push(!ArkTools.isPrototype(protoP1));
conditionArray.push(!ArkTools.isPrototype(protoP2));

// change proto of obj1
// obj11 --> proto --> obj1 --> proto --> protoP1
// obj22 --> proto --> obj1 --> proto --> protoP1
Object.setPrototypeOf(obj1, proxyOfProto);
Object.setPrototypeOf(obj1, proxyOfProto);
conditionArray.push(ArkTools.compareHClass(protoP1, protoP2));
// change proto of obj2
// obj33 --> proto --> obj2 --> proto --> protoP2
Object.setPrototypeOf(obj2, protoP2);
Object.setPrototypeOf(obj3, proxyOfProto);

conditionArray.push(!ArkTools.compareHClass(protoP1, protoP2));
conditionArray.push(!ArkTools.isPrototype(protoP1));
conditionArray.push(ArkTools.isPrototype(protoP2));
conditionArray.push(!ArkTools.isPrototype(proxyOfProto));

test();
obj1["zzz"] = "100zzz";

// check proto
conditionArray.push(obj11.a === "xxx")
conditionArray.push(obj22.a === "xxx")
conditionArray.push(obj11.b === "2b")
conditionArray.push(obj33.a === 1)
conditionArray.push(obj33.c === "3c")
conditionArray.push(obj11.zzz === "100zzz")
conditionArray.push(obj33.zzz === undefined)

let str = "";
// check results
for (let i of conditionArray) {
    if (!i) {
        str = "Test mutli proto ic fail!!!";
        break;
    }
}
if (str == "") {
    str = "Test mutli proto ic success!!!";
}
print(str);
