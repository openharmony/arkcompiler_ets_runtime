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
 * @tc.name:promise
 * @tc.desc:test Promise
 * @tc.type: FUNC
 * @tc.require: issueI5NO8G
 */
var p = new Promise((resolve, reject) => {
    resolve(1479);
})
var p1 = Promise.reject(1357);
var p2 = Promise.resolve(2468);
var p3 = Promise.race([p1, p2]);
p3.then(
    (value) => {
        print("resolve");
        print(value);
    },
    (value) => {
        print("reject");
        print(value);
    }
)

p3.catch((value) => {
    print("catch");
    print(value);
})

var p4 = Promise.all([p, p2]);
p4.then(
    (value) => {
        print("resolve");
        print(value);
    },
    (value) => {
        print("reject");
        print(value);
    }
)

const original = Promise.resolve(33);
const cast = Promise.resolve(original);
cast.then((value) => {
  print(`value:${value}`);
});
print(`original === cast ? ${original === cast}`);

function testCase(description, testFn) {
    print(`test:${description}`);
    try {
        testFn();
        print("success");
    } catch (error) {
        console.error(`fail: ${error.message}`);
    }
}

async function checkResolved(promise, expected) {
    const result = await promise;
    if (result !== expected) {
        throw new Error(`expected ${expected}，actual ${result}`);
    }
}

async function checkRejected(promise, expectedError) {
    try {
        await promise;
        throw new Error("Expected promise rejected, but actually resolved");
    } catch (error) {
        if (error !== expectedError && error.message !== expectedError.message) {
            throw new Error(`expected ${expectedError}，actual ${error}`);
        }
    }
}


async function runTests() {
    testCase("1", async () => {
        const original = Promise.resolve("original");
        await checkResolved(Promise.resolve(original), "original");
    });

    testCase("2", async () => {
        const rejected = Promise.reject("error");
        await checkRejected(Promise.resolve(rejected), "error");
    });

    testCase("3", async () => {
        const thenable = {
            then: function(resolve) {
                resolve("thenable");
            }
        };
        await checkResolved(Promise.resolve(thenable), "thenable");
    });

    testCase("4", async () => {
        const result = Promise.resolve(10)
            .then(x => x * 2)
            .then(x => x + 5);
        await checkResolved(result, 25);
    });

    testCase("5", async () => {
        const asyncAdd = (a, b) =>
            new Promise(resolve => setTimeout(() => resolve(a + b), 10));
        const result = Promise.resolve(10)
            .then(x => asyncAdd(x, 5))
            .then(x => asyncAdd(x, 3));
        await checkResolved(result, 18);
    });
}
runTests();