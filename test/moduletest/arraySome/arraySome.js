/*
 * Copyright (c) 2024 Huawei Device Co., Ltd.
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

const someTestCases = [
    () => {
        let log = [];
        const result = [1, 2, 3].some((x, i, arr) => {
            log.push({ value: x, index: i, array: [...arr] });
            return x > 2;
        });
        print("Log:", JSON.stringify(log), "Result:", JSON.stringify(result)); // Result: true
    },

    () => {
        let log = [];
        const result = [1, 2, 3].some((x, i, arr) => {
            log.push({ value: x, index: i, array: [...arr] });
            return x > 3;
        });
        print("Log:", JSON.stringify(log), "Result:", JSON.stringify(result)); // Result: false
    },

    () => {
        let log = [];
        const result = [].some((x, i, arr) => {
            log.push({ value: x, index: i, array: [...arr] });
            return x > 0;
        });
        print("Log:", JSON.stringify(log), "Result:", JSON.stringify(result)); // Result: false
    },

    () => {
        let log = [];
        const result = [false, 0, ''].some((x, i, arr) => {
            log.push({ value: x, index: i, array: [...arr] });
            return x;
        });
        print("Log:", JSON.stringify(log), "Result:", JSON.stringify(result)); // Result: false
    },

    () => {
        let log = [];
        const result = [1, 2, 3].some((x, i, arr) => {
            log.push({ value: x, index: i, array: [...arr] });
            return x > 1;
        });
        print("Log:", JSON.stringify(log), "Result:", JSON.stringify(result)); // Result: true
    },

    () => {
        let log = [];
        const result = [NaN, 2, 3].some((x, i, arr) => {
            log.push({ value: x, index: i, array: [...arr] });
            return isNaN(x);
        });
        print("Log:", JSON.stringify(log), "Result:", JSON.stringify(result)); // Result: true
    },

    () => {
        let log = [];
        let arr = [1, 2, 3];
        const result = arr.some((x, i, arrRef) => {
            log.push({ value: x, index: i, array: [...arrRef] });
            if (i === 1) arrRef.push(4);
            return x === 4;
        });
        print("Log:", JSON.stringify(log), "Result:", JSON.stringify(result)); // Result: true
    },

    () => {
        let log = [];
        const result = [{ id: 1 }, { id: 2 }].some((x, i, arr) => {
            log.push({ value: x, index: i, array: [...arr] });
            return x.id === 2;
        });
        print("Log:", JSON.stringify(log), "Result:", JSON.stringify(result)); // Result: true
    },

    () => {
        let log = [];
        const context = { value: 2 };
        const result = [1, 2, 3].some(function(x, i, arr) {
            log.push({ value: x, index: i, array: [...arr] });
            return x === this.value;
        }, context);
        print("Log:", JSON.stringify(log), "Result:", JSON.stringify(result)); // Result: true
    },

    () => {
        let log = [];
        const result = [1, 2, 3].some((x, i, arr) => {
            log.push({ value: x, index: i, array: [...arr] });
            return false;
        });
        print("Log:", JSON.stringify(log), "Result:", JSON.stringify(result)); // Result: false
    },

    () => {
        let log = [];
        const result = [1, 2, 3].some((x, i, arr) => {
            log.push({ value: x, index: i, array: [...arr] });
            return true;
        });
        print("Log:", JSON.stringify(log), "Result:", JSON.stringify(result)); // Result: true
    },

    () => {
        let log = [];
        const result = [null, undefined, 0].some((x, i, arr) => {
            log.push({ value: x, index: i, array: [...arr] });
            return x == null;
        });
        print("Log:", JSON.stringify(log), "Result:", JSON.stringify(result)); // Result: true
    },

    () => {
        let log = [];
        let arr = [1, 2, 3];
        const result = arr.some((x, i, arrRef) => {
            log.push({ value: x, index: i, array: [...arrRef] });
            arrRef[i] = x + 1; // 修改当前元素
            return x === 2;
        });
        print("Log:", JSON.stringify(log), "Result:", JSON.stringify(result)); // Result: true
    },

    () => {
        let log = [];
        const result = [[1], [2], [3]].some((x, i, arr) => {
            log.push({ value: x, index: i, array: [...arr] });
            return x[0] === 2;
        });
        print("Log:", JSON.stringify(log), "Result:", JSON.stringify(result)); // Result: true
    },

    () => {
        let log = [];
        const result = [1, 2, 3].some((x, i, arr) => {
            log.push({ value: x, index: i, array: [...arr] });
            return i === 1;
        });
        print("Log:", JSON.stringify(log), "Result:", JSON.stringify(result)); // Result: true
        print("Callback executed count:", log.length); // Should be 2
    },

    () => {
        let log = [];
        const result = [1, 2, -3].some((x, i, arr) => {
            log.push({ value: x, index: i, array: [...arr] });
            return x < 0;
        });
        print("Log:", JSON.stringify(log), "Result:", JSON.stringify(result)); // Result: true
    },

    () => {
        let log = [];
        const result = [].some((x, i, arr) => {
            log.push({ value: x, index: i, array: [...arr] });
            return typeof x === "object";
        });
        print("Log:", JSON.stringify(log), "Result:", JSON.stringify(result)); // Result: false
    },
];

someTestCases.forEach((test, i) => {
    print(`Some Test Case ${i + 1}:`);
    test();
    print('---');
});

