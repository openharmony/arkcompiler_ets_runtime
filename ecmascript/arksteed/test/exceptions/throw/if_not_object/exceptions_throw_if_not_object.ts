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

// Test: throw.ifnotobject throw path.
// The for-of loop calls iterator.next() which must return an ECMA object.
// If next() returns a non-object, throw.ifnotobject throws a TypeError.

class BadIterator {
    private count: number = 0;

    next(): any {
        this.count += 1;
        if (this.count <= 5) {
            return {done: false, value: this.count};  // proper result object
        }
        // BUG: returns a number instead of {done, value} object → throw.ifnotobject
        return this.count;
    }
}

class BadIterable {
    [Symbol.iterator](): BadIterator {
        return new BadIterator();
    }
}

function exception_throw_if_not_object(): number {
    let sum: number = 0;
    let iterable = new BadIterable();
    try {
        for (let x of iterable) {  // throw.ifnotobject
            sum += x;
        }
    } catch (e) {
        return sum;  // Expected: TypeError from throw.ifnotobject
    }
    return -1;
}

ArkTools.arkSteedCompileSync(exception_throw_if_not_object);

print(exception_throw_if_not_object());
