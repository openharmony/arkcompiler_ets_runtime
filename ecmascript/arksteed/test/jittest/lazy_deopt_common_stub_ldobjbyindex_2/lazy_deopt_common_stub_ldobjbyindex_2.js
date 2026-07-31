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

// CommonStub D+E lazy-deopt regression test.
// Operation() first installs a prototype-chain dependency through obj.x.
// The selected CommonStub bytecode is inside a try block whose catch is cold during profiling.
// On later compiled runs, its JS-visible hook can independently invalidate the dependency or throw.
// The catch path and the tail of Operation() must continue in the interpreter with a restored frame.

class Root {}
class Mid extends Root {}
class Leaf extends Mid {}

Root.prototype.x = 1;
let obj = new Leaf();
let shouldInvalidateDependency = false;
let shouldThrow = false;
const REP = 60000;

function invalidateDependency()
{
    if (shouldInvalidateDependency) {
        Mid.prototype.x = 2;
    }
}

function invalidateAndThrow()
{
    if (shouldThrow) {
        throw "boom";
    }
}

const proxyStorage = {k: 10, 0: 20};
const proxyObject = new Proxy(proxyStorage, {
    get(target, key, receiver) {
        invalidateDependency();
        invalidateAndThrow();
        return Reflect.get(target, key, receiver);
    },
    set(target, key, value, receiver) {
        invalidateDependency();
        invalidateAndThrow();
        target[key] = value;
        return true;
    },
    defineProperty(target, key, descriptor) {
        invalidateDependency();
        invalidateAndThrow();
        Object.defineProperty(target, key, descriptor);
        return true;
    },
    deleteProperty(target, key) {
        invalidateDependency();
        invalidateAndThrow();
        return Reflect.deleteProperty(target, key);
    },
    has(target, key) {
        invalidateDependency();
        invalidateAndThrow();
        return Reflect.has(target, key);
    },
    ownKeys(target) {
        invalidateDependency();
        invalidateAndThrow();
        return ["k"];
    },
    getOwnPropertyDescriptor(target, key) {
        return { configurable: true, enumerable: true, value: target[key] };
    }
});

function resetProxyStorage()
{
    delete proxyStorage.field;
    delete proxyStorage.namedMethod;
    proxyStorage.k = 10;
    proxyStorage[0] = 20;
}

function setMode(invalidateDependency, throwException)
{
    shouldInvalidateDependency = invalidateDependency;
    shouldThrow = throwException;
}

function resetState(invalidateDependency, throwException)
{
    setMode(invalidateDependency, throwException);
    delete Mid.prototype.x;
    Root.prototype.x = 1;
    resetProxyStorage();
}

function printCompiledState(label, includeOperation)
{
    print(label + ": Test isCompiled:", ArkTools.arkSteedIsCompiled(Test));
    if (includeOperation) {
        print(label + ": Operation isCompiled:", ArkTools.arkSteedIsCompiled(Operation));
    }
}

function Test(name)
{
    print(name + " state:", Operation());
}

function PreheatOperation()
{
    for (let i = 0; i < REP; i++) {
        Operation();
    }
}

function runCase(name)
{
    resetState(false, false);

    printCompiledState("Before 1st call", true);
    Test(name);
    printCompiledState("After 1st call", true);
    print("----------------");

    resetState(false, true);
    PreheatOperation();

    resetState(false, false);
    ArkTools.arkSteedCompileSync(Operation);
    ArkTools.arkSteedCompileSync(Test);

    setMode(false, true);
    printCompiledState("Before 2nd call", false);
    Test(name);
    printCompiledState("After 2nd call", false);
    print("----------------");

    resetState(false, false);
    Test(name);
    print("----------------");

    resetState(false, false);
    ArkTools.arkSteedCompileSync(Operation);
    ArkTools.arkSteedCompileSync(Test);

    setMode(true, false);
    printCompiledState("Before 3rd call", true);
    Test(name);
    printCompiledState("After 3rd call", true);
    print("----------------");
}

function Operation()
{
    let before = obj.x;
    let marker = before + 10;
    let result = "unset";
    try {
        // The following Proxy operation enters a trap that can invalidate dependencies or throw.
        // That common-stub bytecode site is where the compiled run triggers lazy-deopt.
        result = "ok:" + proxyObject[0];
    } catch (e) {
        result = "caught:" + e;
    }
    let after = obj.x;
    let tail = 0;
    for (let i = 0; i < 3; i++) {
        tail += after + i;
    }
    if (after === 2) {
        tail += 100;
    } else {
        tail += 10;
    }
    return before + ":" + result + ":" + after + ":" + marker + ":" + tail;
}

runCase("lazy_deopt_common_stub_ldobjbyindex_2");
