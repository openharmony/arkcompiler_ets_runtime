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
 * @tc.name:callframe
 * @tc.desc:test different kinds of call frames
 * @tc.type: FUNC
 * @tc.require: issueI5NO8G
 */
// normal
function foo0(a) {
    return a;
}

// haveThis
function foo1(a) {
    if (!this) {
        return a;
    }
    this.a = a;
}

// haveNewTarget
function foo2(a) {
    if (!new.target) {
        return a;
    }
}

// haveThis, haveNewTarget
function foo3(a) {
    if (!new.target) {
        return a;
    }
    this.a = a;
}

// haveExtra
function foo4(a, ...args) {
    return args[a];
}

// haveThis, haveExtra
function foo5(a, ...args) {
    if (!this) {
        return a;
    }
    this.a = args[a];
}

// haveNewTarget, haveExtra
function foo6(a, ...args) {
    if (!new.target) {
        return args[a];
    }
}

// haveThis, haveNewTarget, haveExtra
function foo7(a, ...args) {
    if (!new.target) {
        return args[a];
    }
    this.a = args[a];
}

// arguments
function foo8() {
    return arguments;
}

print(foo0(1,2,3));
print(foo1(1,2,3));  // 'this' is undefined in strict mode
print(foo2(1,2,3));
print(foo3(1,2,3));
print(foo4(1,2,3));
print(foo5(1,2,3));  // 'this' is undefined in strict mode
print(foo6(1,2,3));
print(foo7(1,2,3));
print(new foo0(1,2,3).a);
print(new foo1(1,2,3).a);
print(new foo2(1,2,3).a);
print(new foo3(1,2,3).a);
print(new foo4(1,2,3).a);
print(new foo5(1,2,3).a);
print(new foo6(1,2,3).a);
print(new foo7(1,2,3).a);
print("arguments");
var arg = foo8(1,2,3);
print(arg.length);
print(arg[0]);
print(arg[1]);
print(arg[2]);
print(arg[3]);