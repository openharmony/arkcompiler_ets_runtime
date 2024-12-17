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


const array1 = [5, 12, 8, 130, 44];
const found = array1.find((element) => element > 10);
print(found);

const arrayLike = {
	  length: 3,
	  0: 2,
	  1: 7.3,
	  2: 4,
};
print(Array.prototype.find.call(arrayLike, (x) => !Number.isInteger(x)));

const array = [0, 1, , , , 5, 6];

array.find((value, index) => {
    print(`${index},${value}`);
});

array.find((value, index) => {
    if (index === 0) {
        print(`array[5]${array[5]}`);
        delete array[5];
    }
    print(`${index},${value}`);
});

function isPrime(element, index, array) {
	  let start = 2;
	  while (start <= Math.sqrt(element)) {
		      if (element % start++ < 1) {
			            return false;
			          }
		    }
	  return element > 1;
}

print([4, 6, 8, 12].find(isPrime));
print([4, 5, 8, 12].find(isPrime));
//
const numbers = [3, -1, 1, 4, 1, 5, 9, 2, 6];
const firstTrough = numbers
  .filter((num) => num > 0)
  .find((num, idx, arr) => {
    if (idx > 0 && num >= arr[idx - 1]) return false;
    if (idx < arr.length - 1 && num >= arr[idx + 1]) return false;
    return true;
  });
print(firstTrough); // 1

const words = ["spray", "limit", "limits"];
const deleteWords = words.find((word, index, arr) => {
  arr.length=4
  word="asd"
  return word == "asd"
});
print(deleteWords);
print(words.length);

const findTestCases = [
    () => {
        let log = [];
        const result = [1, 2, 3, 4].find((x, i, arr) => {
            log.push({value: x, index: i, array: [...arr]});
            return x === 3;
        });
        print("Log:", JSON.stringify(log), "Result:", JSON.stringify(result)); // Result: 3
    },

    () => {
        let log = [];
        const result = [1, 2, 3, 4].find((x, i, arr) => {
            log.push({value: x, index: i, array: [...arr]});
            return x === 5;
        });
        print("Log:", JSON.stringify(log), "Result:", JSON.stringify(result)); // Result: undefined
    },

    () => {
        let log = [];
        const result = [].find((x, i, arr) => {
            log.push({value: x, index: i, array: [...arr]});
            return x > 0;
        });
        print("Log:", JSON.stringify(log), "Result:", JSON.stringify(result)); // Result: undefined
    },

    () => {
        let log = [];
        const result = [false, 0, ''].find((x, i, arr) => {
            log.push({value: x, index: i, array: [...arr]});
            return !x; // 找到 falsy 值
        });
        print("Log:", JSON.stringify(log), "Result:", JSON.stringify(result)); // Result: false
    },

    () => {
        let log = [];
        const result = [1, 2, 3, 4].find((x, i, arr) => {
            log.push({value: x, index: i, array: [...arr]});
            return x > 2; // 第一个大于 2 的元素
        });
        print("Log:", JSON.stringify(log), "Result:", JSON.stringify(result)); // Result: 3
    },

    () => {
        let log = [];
        const result = [null, 1, 2].find((x, i, arr) => {
            log.push({value: x, index: i, array: [...arr]});
            return x !== null && x !== undefined;
        });
        print("Log:", JSON.stringify(log), "Result:", JSON.stringify(result)); // Result: 1
    },

    () => {
        let log = [];
        let arr = [1, 2, 3];
        const result = arr.find((x, i, arrRef) => {
            log.push({value: x, index: i, array: [...arrRef]});
            if (i === 1) arrRef.push(4); // 修改数组
            return x === 3;
        });
        print("Log:", JSON.stringify(log), "Result:", JSON.stringify(result)); // Result: 3
    },

    () => {
        let log = [];
        const result = [NaN, 1, 2].find((x, i, arr) => {
            log.push({value: x, index: i, array: [...arr]});
            return !isNaN(x);
        });
        print("Log:", JSON.stringify(log), "Result:", JSON.stringify(result)); // Result: 1
    },

    () => {
        let log = [];
        const result = [1, 2, 3].find((x, i, arr) => {
            log.push({value: x, index: i, array: [...arr]});
            return false;
        });
        print("Log:", JSON.stringify(log), "Result:", JSON.stringify(result)); // Result: undefined
    },

    () => {
        let log = [];
        const result = [1, 2, 3].find((x, i, arr) => {
            log.push({value: x, index: i, array: [...arr]});
            return true; // 返回第一个元素
        });
        print("Log:", JSON.stringify(log), "Result:", JSON.stringify(result)); // Result: 1
    },

    () => {
        let log = [];
        const context = {value: 3};
        const result = [1, 2, 3, 4].find(function (x, i, arr) {
            log.push({value: x, index: i, array: [...arr]});
            return x === this.value; // 使用 this 进行比较
        }, context);
        print("Log:", JSON.stringify(log), "Result:", JSON.stringify(result)); // Result: 3
    },

    () => {
        let log = [];
        const result = [{id: 1}, {id: 2}].find((x, i, arr) => {
            log.push({value: x, index: i, array: [...arr]});
            return x.id === 2;
        });
        print("Log:", JSON.stringify(log), "Result:", JSON.stringify(result)); // Result: { id: 2 }
    },

    () => {
        let log = [];
        let obj = {x: 1};
        const result = [obj, obj].find((x, i, arr) => {
            log.push({value: x, index: i, array: [...arr]});
            x.x++; // 修改对象属性
            return x.x > 1;
        });
        print("Log:", JSON.stringify(log), "Result:", JSON.stringify(result)); // Result: { x: 2 }
    },

    () => {
        let log = [];
        const result = [].find((x, i, arr) => {
            log.push({value: x, index: i, array: [...arr]});
            return x === 1;
        });
        print("Log:", JSON.stringify(log), "Result:", JSON.stringify(result)); // Result: undefined
    },

    () => {
        let log = [];
        let arr = [1, 2, 3];
        const result = arr.find((x, i, arrRef) => {
            log.push({value: x, index: i, array: [...arrRef]});
            if (i === 1) arrRef.splice(1, 1, 4); // 修改数组
            return x === 4;
        });
        print("Log:", JSON.stringify(log), "Result:", JSON.stringify(result)); // Result: 4
    },
];

findTestCases.forEach((test, i) => {
    print(`Find Test case ${i + 1}:`);
    test();
    print('---');
});
