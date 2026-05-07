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
// 初始化数组
let fruitArray = [];

function operate() {
    for (let i = 0; i < 20; i++) {
        const apple = {
            type: `banana`,
            field2: i + 1,
            field3: i % 2 === 0,
            field4: [`Field4-${i}`],
            field5: { colorString: `ColorString-${i}`, colorInt: i },
            field6: new Date(),
            field7: `Field7-${i}`,
            field8: i % 3 === 0 ? null : `Field8-${i}`,
            field9: [i, i * 2, i * 3],
            field10: i % 4 === 0,
            additionalField1: `AdditionalField1-${i}`,
            additionalField2: i + 11,
            additionalField3: i % 5 === 0,
            additionalField4: [`AdditionalField4-${i}`],
            additionalField5: { colorString: `AdditionalColorString-${i}`, colorInt: i + 5 },
            additionalField6: new Date(),
            additionalField7: `AdditionalField7-${i}`,
            additionalField8: i % 2 === 0 ? null : `AdditionalField8-${i}`,
            additionalField9: [i + 1, (i + 1) * 2, (i + 1) * 3],
            additionalField10: i % 3 === 0
        };
        fruitArray.push(apple);
    }
    for (let i = 0; i < 20; i++) {
        const apple = {
            type: `apple`,
            field2: i + 1,
            field3: i % 2 === 0,
            field4: [`Field4-${i}`],
            field5: { colorString: `ColorString-${i}`, colorInt: i },
            field6: new Date(),
            field7: `Field7-${i}`,
            field8: i % 3 === 0 ? null : `Field8-${i}`,
            field9: [i, i * 2, i * 3],
            field10: i % 4 === 0,
            additionalField1: `AdditionalField1-${i}`,
            additionalField2: i + 11,
            additionalField3: i % 5 === 0,
            additionalField4: [`AdditionalField4-${i}`],
            additionalField5: { colorString: `AdditionalColorString-${i}`, colorInt: i + 5 },
            additionalField6: new Date(),
            additionalField7: `AdditionalField7-${i}`,
            additionalField8: i % 2 === 0 ? null : `AdditionalField8-${i}`,
            additionalField9: [i + 1, (i + 1) * 2, (i + 1) * 3],
            additionalField10: i % 3 === 0
        };
        fruitArray.push(apple);
    }
    for (let i = 0; i < 20; i++) {
        const apple = {
            type: `orange`,
            field2: i + 1,
            field3: i % 2 === 0,
            field4: [`Field4-${i}`],
            field5: { colorString: `ColorString-${i}`, colorInt: i },
            field6: new Date(),
            field7: `Field7-${i}`,
            field8: i % 3 === 0 ? null : `Field8-${i}`,
            field9: [i, i * 2, i * 3],
            field10: i % 4 === 0,
            additionalField1: `AdditionalField1-${i}`,
            additionalField2: i + 11,
            additionalField3: i % 5 === 0,
            additionalField4: [`AdditionalField4-${i}`],
            additionalField5: { colorString: `AdditionalColorString-${i}`, colorInt: i + 5 },
            additionalField6: new Date(),
            additionalField7: `AdditionalField7-${i}`,
            additionalField8: i % 2 === 0 ? null : `AdditionalField8-${i}`,
            additionalField9: [i + 1, (i + 1) * 2, (i + 1) * 3],
            additionalField10: i % 3 === 0
        };
        fruitArray.push(apple);
    }
    for (let i = 0; i < 20; i++) {
        const apple = {
            type: `grape`,
            field2: i + 1,
            field3: i % 2 === 0,
            field4: [`Field4-${i}`],
            field5: { colorString: `ColorString-${i}`, colorInt: i },
            field6: new Date(),
            field7: `Field7-${i}`,
            field8: i % 3 === 0 ? null : `Field8-${i}`,
            field9: [i, i * 2, i * 3],
            field10: i % 4 === 0,
            additionalField1: `AdditionalField1-${i}`,
            additionalField2: i + 11,
            additionalField3: i % 5 === 0,
            additionalField4: [`AdditionalField4-${i}`],
            additionalField5: { colorString: `AdditionalColorString-${i}`, colorInt: i + 5 },
            additionalField6: new Date(),
            additionalField7: `AdditionalField7-${i}`,
            additionalField8: i % 2 === 0 ? null : `AdditionalField8-${i}`,
            additionalField9: [i + 1, (i + 1) * 2, (i + 1) * 3],
            additionalField10: i % 3 === 0
        };
        ;
        fruitArray.push(apple);
    }
}

function jsTest() {
    resetVars();
    let startTime = new Date().getTime();
    for (let index = 0; index < 300; index++) {
        operate()
    }
    let result = fruitArray.reduce((pre, cur) => {
        return pre + cur.field2
    }, 0)

    let result2 = fruitArray.filter(fruit =>!(fruit.type == 'apple')).reduce((pre, cur) => {
        return pre + cur.additionalField2
    }, 0);
    let result3 = fruitArray.filter(fruit =>!(fruit.type == 'orange')).reduce((pre, cur) => {
        return pre + cur.additionalField5.colorInt
    }, 0);
    let endTime = new Date().getTime();
    print(`bili: ms = ${endTime-startTime}`)
}

function resetVars() {
    fruitArray = []
}

let loopCountForPreheat = 1;
for (let i = 0; i < loopCountForPreheat; i++) {
    jsTest()
}

function sleep(delay) {
    const start = Date.now();
    while (Date.now() - start < delay) {}
}

sleep(5000);

jsTest()