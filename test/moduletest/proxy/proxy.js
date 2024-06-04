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
 * @tc.name:proxy
 * @tc.desc:test Proxy
 * @tc.type: FUNC
 * @tc.require: issueI5NO8G
 */
function add(a, b) {
    return a + b
}

let addProxy = new Proxy(add, {
    apply: function(target, thisObj, args) {
        return target.apply(thisObj, args)
    }
})

for (var a = 1; a < 4; a++) {
    print(addProxy(a, 12345 * a) == add(12345 * a, a))
}

class EmployeeEntity {
}
let obj = new Proxy(EmployeeEntity,{});
let arr = [obj];
let set = new Set(arr);
print(set.has(obj));

const a1 = [-122934378, 536870889, -4294967295, -8, 11];
const a2 = {
};
const a3 = new Proxy(a1, a2);
Object.freeze(a3);
print(Object.isFrozen(a3));

// CreateObjectWithExcludedKeysDealWithProxy
const v0 = {
    value : 'name'
};
const v5 = new Proxy(v0, {});
const {...v1} = v5;
print(v5.value);
print(v1.value);
print(v0.value);
const arr2 = [1, 2, 3];
const handler = {};
const proxy = new Proxy(arr2, handler);
const arr3 = proxy.constructor(1, 2, 3, 4);
print(arr3);

function Target() {}
var handler1 = {
    construct: function(t, args) {
        return new t(args[0], args[1]);
    }
};
var P = new Proxy(Target, handler1);
new P(1, 1.1);
print(JSON.stringify(P));

try {
    const v0 = /\011+/iyd;
    const v2 = new Uint8ClampedArray(v0, Uint8ClampedArray, Uint8ClampedArray);
    const v5 = new Proxy({}, {});
    new v5(...v2);
} catch (e) {
    print("constructor is fail");
}
const v3 = new Uint8ClampedArray(WeakMap);
const o3 = {
};
const v6 = new Proxy(v3, o3);
try {
    new WeakMap([v6]);
} catch (error) {
    print(error.name);
}

let arr1 = new Array;
let pro1 = new Proxy(arr1, {});
print(Object.prototype.toString.call(pro1));


{
    const v0 = [1.0,2.2250738585072014e-308,-7.829944258836235e+307,-7.46187308415638,-413646.9640026712,5.525707214803659e+305,-1000000000000.0];
    const o1 = {
    };
    const v3 = new Proxy(v0, o1);

    function F4(a6, a7) {
        const v9 = new Proxy(RegExp, v3);
        new v9(a6, v9, F4);
    }
    try {
        new F4();
    } catch (error) {
        print(error);
    }
}
class C9{};
let v4 = new Proxy(Uint32Array,Reflect);
class C4 extends v4 {
  constructor(a6,a7,a8){
    super();
    new C9(a7,C4);
  }
}
new C4();
print("test proxy constructor success!")

var proxyHandler = {};
var object = new Proxy({}, proxyHandler);
print(Object.prototype.isPrototypeOf.call(object, object));

var target3 = {
    get attr() {
        return this;
    },
    name:"kkk"
};
var p = new Proxy(target3, {
    get: null
});
print(p.attr.name);

var target4 = {
    name:"kkk"
};
var p1 = new Proxy(target4, {
    get(target, property) {
        return 1;
    }
});
Object.defineProperty(target4, "age", {
    writable:false,
    value : 33,
    configurable:false
});
try{
    p1.age;
} catch (e) {
    print(e);
}

var p2 = new Proxy(target4, {});
print(p2.name);

var target5 = {
    name:"kkk",
};
Object.defineProperty(target5, "age", {
    configurable:false,
    set : function (val) {
        return 4;
    }
});
var p3 = new Proxy(target5, {
    get(target, property) {
        return 4;
    }
});
try {
    print(p3.age);
} catch(e) {
    print(e);
}

var target6 = {
    name:"kkk",
};
Object.defineProperty(target6, "age", {
    configurable:false,
    get : function (val) {
        return 4;
    }
});
var p4 = new Proxy(target6, {
    get(target, property) {
        return 4;
    },
    set(target, property, value) {
        return 6;
    }
});
try {
    p4.age = 9;
} catch(e) {
    print(e);
}

var target7 = {
    name:"kkk",
};
Object.defineProperty(target7, "age", {
    writable:false,
    value : 33,
    configurable:false
});
var p5 = new Proxy(target7, {
    get(target, property) {
        return 4;
    },
    set(target, property, value) {
        return 4;
    }
});
try {
    p5.age = 9;
} catch(e) {
    print(e);
}

var target8 = {
    name:"kkk",
};

var p6 = new Proxy(target8, {
    get(target, property) {
        return 4;
    },
    set(target, property, value) {
        return false;
    }
});
try {
    p6.name = 9;
} catch(e) {
    print(e);
}

var target9 = {
    name:"kkk",
};

var p7 = new Proxy(target9, {
    get(target, property) {
        return target[property];
    },
    set(target, property, value) {
        target[property] = value;
        return true;
    }
});


var obj1 = {};
obj1.__proto__ = p7;
print(obj1.name);

try {
    obj1.name = "wode";
    print(obj1.name);
} catch(e) {
    print(e);
}

try {
    new Proxy(undefined, undefined);
} catch(e) {
    print(e);
}

try {
    new Proxy({}, undefined);
} catch(e) {
    print(e);
}
