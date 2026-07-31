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

// RuntimeStub dependency lazy-deopt regression test.
// Operation() first installs a prototype-chain dependency through obj.x.
// The selected RuntimeStub bytecode then invalidates that dependency.
// The tail of Operation() must continue in the interpreter and observe the updated frame state.

class Root {}
class Mid extends Root {}
class Leaf extends Mid {}

Root.prototype.x = 1;
let obj = new Leaf();
let shouldInvalidate = false;

function invalidateDependency()
{
    if (shouldInvalidate) {
        Mid.prototype.x = 2;
    }
}

function resetState()
{
    shouldInvalidate = false;
    delete Mid.prototype.x;
    Root.prototype.x = 1;
}

function printCompiledState(label)
{
    print(label + ": Test isCompiled:", ArkTools.arkSteedIsCompiled(Test));
    print(label + ": Operation isCompiled:", ArkTools.arkSteedIsCompiled(Operation));
}

function Test(name)
{
    print(name + " state:", Operation());
}

function runCase(name)
{
    resetState();

    printCompiledState("Before 1st call");
    Test(name);
    printCompiledState("After 1st call");
    print("----------------");

    resetState();
    ArkTools.arkSteedCompileSync(Operation);
    ArkTools.arkSteedCompileSync(Test);

    shouldInvalidate = true;
    printCompiledState("Before 2nd call");
    Test(name);
    printCompiledState("After 2nd call");
    print("----------------");
}

class BaseForSuper {
    get k() {
        invalidateDependency();
        return 9;
    }
    set k(value) {
        invalidateDependency();
        this.saved = value;
    }
}
class SuperReceiver extends BaseForSuper {
    ldByName() {
        return super.k;
    }
    stByName(value) {
        super.k = value;
        return this.saved;
    }
    ldByValue(key) {
        return super[key];
    }
    stByValue(key, value) {
        super[key] = value;
        return this.saved;
    }
}
const superReceiver = new SuperReceiver();

function Operation()
{
    let before = obj.x;
    let marker = before + 10;
    // The following expression triggers super keyed load invokes the BaseForSuper getter.
    // That runtime-stub bytecode site is where the compiled run triggers lazy-deopt.
    let result = superReceiver.ldByValue("k");
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

runCase("lazy_deopt_runtime_stub_ldsuperbyvalue_1");
