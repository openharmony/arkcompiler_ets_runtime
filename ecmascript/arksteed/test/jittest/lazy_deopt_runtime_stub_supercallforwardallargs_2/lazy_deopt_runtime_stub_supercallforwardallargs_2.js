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

// RuntimeStub dependency lazy-deopt regression test for super-call bytecodes.
// The derived constructor reads obj.x before the selected super-call form. The base
// constructor then mutates the prototype dependency, so the compiled constructor must
// lazy-deopt and continue after super() with the restored frame state.

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

class BaseCtor {
    constructor(value) {
        invalidateDependency();
        invalidateAndThrow();
        this.baseValue = value;
    }
}

class ForwardCtor extends BaseCtor {}
class SubjectCtor extends ForwardCtor {
    constructor(value) {
        let before = obj.x;
        let marker = before + 10;
        // The default constructor in ForwardCtor uses CALLRUNTIME_SUPERCALLFORWARDALLARGS;
        // BaseCtor mutates the dependency while the super chain is active.
        super(value);
        let after = obj.x;
        this.state = before + ":" + this.baseValue + ":" + after + ":" + marker;
    }
}


function printCompiledState(label, includeOperation)
{
    print(label + ": Test isCompiled:", ArkTools.arkSteedIsCompiled(Test));
    print(label + ": SubjectCtor isCompiled:", ArkTools.arkSteedIsCompiled(SubjectCtor));
    if (includeOperation) {
        print(label + ": Operation isCompiled:", ArkTools.arkSteedIsCompiled(Operation));
    }
}

function Operation()
{
    try {
        let instance = new SubjectCtor(7);
        return instance.state;
    } catch (e) {
        return "caught:" + e;
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

function compileTargets()
{
    ArkTools.arkSteedCompileSync(SubjectCtor);
    ArkTools.arkSteedCompileSync(Operation);
    ArkTools.arkSteedCompileSync(Test);
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
    compileTargets();

    setMode(false, true);
    printCompiledState("Before 2nd call", false);
    Test(name);
    printCompiledState("After 2nd call", false);
    print("----------------");

    resetState(false, false);
    Test(name);
    print("----------------");

    resetState(false, false);
    compileTargets();

    setMode(true, false);
    printCompiledState("Before 3rd call", true);
    Test(name);
    printCompiledState("After 3rd call", true);
    print("----------------");
}

runCase("lazy_deopt_runtime_stub_supercallforwardallargs_2");
