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

class LogEntry {
    value;
    index;
    array;
}
const testCases = [
    () => {
        let log = [];
        const result = [1, 2, 3, 4].every((x, i, arr) => {
            log.push({ value: x, index: i, array: [...arr] });
            return x > 0;
        });
        print("Log:", JSON.stringify(log), "Result:", result);
    },

    () => {
        let log = [];
        const result = [1, 2, -3, 4].every((x, i, arr) => {
            log.push({ value: x, index: i, array: [...arr] });
            return x > 0;
        });
        print("Log:", JSON.stringify(log), "Result:", result);
    },

    () => {
        let log = [];
        const result = [].every((x, i, arr) => {
            log.push({ value: x, index: i, array: [...arr] });
            return x > 0;
        });
        print("Log:", JSON.stringify(log), "Result:", result);
    },

    () => {
        let log = [];
        const result = [10].every((x, i, arr) => {
            log.push({ value: x, index: i, array: [...arr] });
            return x > 0;
        });
        print("Log:", JSON.stringify(log), "Result:", result);
    },

    () => {
        let log = [];
        const result = [, , 3, , 5].every((x, i, arr) => {
            log.push({ value: x, index: i, array: [...arr] });
            return x !== undefined;
        });
        print("Log:", JSON.stringify(log), "Result:", result);
    },

    () => {
        let log = [];
        const result = [1, undefined, 3].every((x, i, arr) => {
            log.push({ value: x, index: i, array: [...arr] });
            return x > 0;
        });
        print("Log:", JSON.stringify(log), "Result:", result);
    },

    () => {
        let log = [];
        const result = [null, 1, 2].every((x, i, arr) => {
            log.push({ value: x, index: i, array: [...arr] });
            return x !== null;
        });
        print("Log:", JSON.stringify(log), "Result:", result);
    },

    () => {
        let log = [];
        const result = [NaN, 1, 2].every((x, i, arr) => {
            log.push({ value: x, index: i, array: [...arr] });
            return !isNaN(x);
        });
        print("Log:", JSON.stringify(log), "Result:", result);
    },

    () => {
        let log = [];
        let arr = [1, 2];
        const result = arr.every((x, i, arrRef) => {
            log.push({ value: x, index: i, array: [...arrRef] });
            if (i === 1) arrRef.push(3);
            return x > 0;
        });
        print("Log:", JSON.stringify(log), "Result:", result);
    },

    () => {
        let log = [];
        let arr = [1, 2, 3];
        const result = arr.every((x, i, arrRef) => {
            log.push({ value: x, index: i, array: [...arrRef] });
            if (i === 1) arrRef.pop();
            return x > 0;
        });
        print("Log:", JSON.stringify(log), "Result:", result);
    },

    () => {
        let log = [];
        const result = [false, 0, ''].every((x, i, arr) => {
            log.push({ value: x, index: i, array: [...arr] });
            return !x;
        });
        print("Log:", JSON.stringify(log), "Result:", result);
    },

    () => {
        let log = [];
        const result = [1, 2, 3].every((x, i, arr) => {
            log.push({ value: x, index: i, array: [...arr] });
            return true;
        });
        print("Log:", JSON.stringify(log), "Result:", result);
    },

    () => {
        let log = [];
        const result = [1, 2, 3].every((x, i, arr) => {
            log.push({ value: x, index: i, array: [...arr] });
            return false;
        });
        print("Log:", JSON.stringify(log), "Result:", result);
    },

    () => {
        let log = [];
        const result = [[1, 2], [3, 4]].every((subArray, i, arr) => {
            log.push({ value: subArray, index: i, array: [...arr] });
            return subArray.every(x => x > 0);
        });
        print("Log:", JSON.stringify(log), "Result:", result);
    },

    () => {
        let log = [];
        let obj = { x: 1 };
        const result = [obj, obj].every((x, i, arr) => {
            log.push({ value: x, index: i, array: [...arr] });
            x.x++;
            return x.x > 1;
        });
        print("Log:", JSON.stringify(log), "Result:", result);
    },

    () => {
        let log = [];
        const context = { limit: 5 };
        const result = [1, 2, 3].every(function (x, i, arr) {
            log.push({ value: x, index: i, array: [...arr] });
            return x < this.limit;
        }, context);
        print("Log:", JSON.stringify(log), "Result:", result);
    },

    () => {
        let log = [];
        try {
            [1, 2, 3].every((x, i, arr) => {
                log.push({ value: x, index: i, array: [...arr] });
                if (x === 2) throw new Error('Test Error');
                return x > 0;
            });
        } catch (e) {
            print("Log:", JSON.stringify(log), "Error:", e.message);
        }
    },

    () => {
        let log = [];
        const result = [{ id: 1 }, { id: 2 }].every((x, i, arr) => {
            log.push({ value: x, index: i, array: [...arr] });
            return x.id > 0;
        });
        print("Log:", JSON.stringify(log), "Result:", result);
    },

    // Callback modifies the array during traversal (removing elements then add)
    () => {
        let log = [];
        let arr = [1, 2, 3, 4];
        const result = arr.every((x, i, arrRef) => {
            log.push({ value: x, index: i, array: [...arrRef] });
            if (i === 1) arrRef.pop(); // Remove the last element
            if (i === 2) arrRef.push(5); // Add the last element
            return x > 0;
        });
        print("Log:", JSON.stringify(log), "Result:", result);
    },
];

testCases.forEach((test, i) => {
    print(`Test case ${i + 1}:`);
    test();
    print('---');
});
