/*
 * Copyright (c) 2023 Huawei Device Co., Ltd.
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
 * @tc.name:arrayfindIndex
 * @tc.desc:test Array.findIndex
 * @tc.type: FUNC
 * @tc.require: issueI8D8UH
 */

(function(){
    var a = [0, 1,,3];
    var index = a.findIndex(function(val){
        return val === undefined;
    });
    print(index);
})();

(function () {
    var array1 = [,];

    function findIndex() {
      return array1.findIndex(v => v > 0);
    }
    array1.__proto__.push(6);
    print(findIndex())
})();

{
    class C1 {
        [28262n] = 28262n;
    }
    const v2 = new C1();
    const v3 = [-4.869758437495864e+307,1000000000000.0];
    function f4(a5, a6) {
        for (const v7 in v2) {
        }
        a6();
        return a6;
    }
    try { v3.findIndex(f4);} catch (err) { print(err)};
}

const findIndexTestCases = [
    () => {
        let log = [];
        const result = [1, 2, 3, 4].findIndex((x, i, arr) => {
            log.push({ value: x, index: i, array: [...arr] });
            return x === 3;
        });
       print("Log:", JSON.stringify(log), "Result:", JSON.stringify(result)); // Result: 2
    },

    () => {
        let log = [];
        const result = [1, 2, 3, 4].findIndex((x, i, arr) => {
            log.push({ value: x, index: i, array: [...arr] });
            return x === 5;
        });
       print("Log:", JSON.stringify(log), "Result:", JSON.stringify(result)); // Result: -1
    },

    () => {
        let log = [];
        const result = [].findIndex((x, i, arr) => {
            log.push({ value: x, index: i, array: [...arr] });
            return x > 0;
        });
       print("Log:", JSON.stringify(log), "Result:", JSON.stringify(result)); // Result: -1
    },

    () => {
        let log = [];
        const result = [false, 0, ''].findIndex((x, i, arr) => {
            log.push({ value: x, index: i, array: [...arr] });
            return !x; // 查找第一个 falsy 值
        });
       print("Log:", JSON.stringify(log), "Result:", JSON.stringify(result)); // Result: 0
    },

    () => {
        let log = [];
        const result = [1, 2, 3, 4].findIndex((x, i, arr) => {
            log.push({ value: x, index: i, array: [...arr] });
            return x > 2; // 第一个大于 2 的元素
        });
       print("Log:", JSON.stringify(log), "Result:", JSON.stringify(result)); // Result: 2
    },

    () => {
        let log = [];
        const result = [null, 1, 2].findIndex((x, i, arr) => {
            log.push({ value: x, index: i, array: [...arr] });
            return x !== null && x !== undefined; // 查找非 null 和 undefined 的值
        });
       print("Log:", JSON.stringify(log), "Result:", JSON.stringify(result)); // Result: 1
    },

    () => {
        let log = [];
        let arr = [1, 2, 3];
        const result = arr.findIndex((x, i, arrRef) => {
            log.push({ value: x, index: i, array: [...arrRef] });
            if (i === 1) arrRef.push(4); // 修改数组
            return x === 3;
        });
       print("Log:", JSON.stringify(log), "Result:", JSON.stringify(result)); // Result: 2
    },

    () => {
        let log = [];
        const result = [NaN, 1, 2].findIndex((x, i, arr) => {
            log.push({ value: x, index: i, array: [...arr] });
            return !isNaN(x); // 查找第一个非 NaN 元素
        });
       print("Log:", JSON.stringify(log), "Result:", JSON.stringify(result)); // Result: 1
    },

    () => {
        let log = [];
        const result = [1, 2, 3].findIndex((x, i, arr) => {
            log.push({ value: x, index: i, array: [...arr] });
            return false; // 所有值都不满足条件
        });
       print("Log:", JSON.stringify(log), "Result:", JSON.stringify(result)); // Result: -1
    },

    () => {
        let log = [];
        const result = [1, 2, 3].findIndex((x, i, arr) => {
            log.push({ value: x, index: i, array: [...arr] });
            return true; // 第一个元素匹配
        });
       print("Log:", JSON.stringify(log), "Result:", JSON.stringify(result)); // Result: 0
    },

    () => {
        let log = [];
        const context = { value: 3 };
        const result = [1, 2, 3, 4].findIndex(function(x, i, arr) {
            log.push({ value: x, index: i, array: [...arr] });
            return x === this.value; // 使用 this 进行比较
        }, context);
       print("Log:", JSON.stringify(log), "Result:", JSON.stringify(result)); // Result: 2
    },

    () => {
        let log = [];
        const result = [{ id: 1 }, { id: 2 }].findIndex((x, i, arr) => {
            log.push({ value: x, index: i, array: [...arr] });
            return x.id === 2; // 查找 id 为 2 的对象
        });
       print("Log:", JSON.stringify(log), "Result:", JSON.stringify(result)); // Result: 1
    },

    () => {
        let log = [];
        let obj = { x: 1 };
        const result = [obj, obj].findIndex((x, i, arr) => {
            log.push({ value: x, index: i, array: [...arr] });
            x.x++; // 修改对象属性
            return x.x > 1; // 查找对象属性大于 1
        });
       print("Log:", JSON.stringify(log), "Result:", JSON.stringify(result)); // Result: 0
    },

    () => {
        let log = [];
        const result = [].findIndex((x, i, arr) => {
            log.push({ value: x, index: i, array: [...arr] });
            return x === 1;
        });
       print("Log:", JSON.stringify(log), "Result:", JSON.stringify(result)); // Result: -1
    },


    () => {
        let log = [];
        let arr = [1, 2, 3];
        const result = arr.findIndex((x, i, arrRef) => {
            log.push({ value: x, index: i, array: [...arrRef] });
            if (i === 1) arrRef.splice(1, 1, 4); // 修改数组
            return x === 4; // 查找新元素 4
        });
       print("Log:", JSON.stringify(log), "Result:", JSON.stringify(result)); // Result: 1
    },

    () => {
        let log = [];
        const result = [[1], [2], [3]].findIndex((x, i, arr) => {
            log.push({ value: x, index: i, array: [...arr] });
            return x[0] === 2; // 查找数组中嵌套数组值为 2 的索引
        });
       print("Log:", JSON.stringify(log), "Result:", JSON.stringify(result)); // Result: 1
    },

    () => {
        let log = [];
        const result = [1, 2, 3, -1, 5].findIndex((x, i, arr) => {
            log.push({ value: x, index: i, array: [...arr] });
            return x < 0;
        });
       print("Log:", JSON.stringify(log), "Result:", JSON.stringify(result)); // Result: 3
    },

    () => {
        let log = [];
        const result = ['apple', 'banana', 'cherry'].findIndex((x, i, arr) => {
            log.push({ value: x, index: i, array: [...arr] });
            return x === 'banana';
        });
       print("Log:", JSON.stringify(log), "Result:", JSON.stringify(result)); // Result: 1
    },

    () => {
        let log = [];
        const result = [{ name: 'Alice' }, { name: 'Bob' }].findIndex((x, i, arr) => {
            log.push({ value: x, index: i, array: [...arr] });
            return x.name === 'Bob';
        });
       print("Log:", JSON.stringify(log), "Result:", JSON.stringify(result)); // Result: 1
    },

    () => {
        let log = [];
        const result = [{ age: 30 }, { age: 25 }, { age: 30 }].findIndex((x, i, arr) => {
            log.push({ value: x, index: i, array: [...arr] });
            return x.age === 30;
        });
       print("Log:", JSON.stringify(log), "Result:", JSON.stringify(result)); // Result: 0
    },
];

findIndexTestCases.forEach((test, i) => {
   print(`FindIndex Test case ${i + 1}:`);
    test();
   print('---');
});
