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

// RuntimeStub D+E lazy-deopt regression test.
// Operation() first installs a prototype-chain dependency through obj.x.
// The selected RuntimeStub bytecode is inside a try block whose catch is cold during profiling.
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

    resetState(false, false);
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

function ConstructTarget(a, b)
{
    invalidateDependency();
    invalidateAndThrow();
    this.value = a + b + 1;
}

const ctorArgs = [2, 4];

function Operation()
{
    let before = obj.x;
    let marker = before + 10;
    let result = "unset";
    try {
        // The following expression triggers spread construction enters a runtime constructor path.
        // That runtime-stub bytecode site is where the compiled run triggers lazy-deopt.
        result = "ok:" + ((new ConstructTarget(...ctorArgs)).value);
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

runCase("lazy_deopt_runtime_stub_newobjapply_2");
