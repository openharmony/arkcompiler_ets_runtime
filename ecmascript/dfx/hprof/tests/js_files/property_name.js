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

let testObj = {
    id: 12345,
    name: 'TestObject',
    score: 95.5,
    isActive: true,
    tags: ['tag1', 'tag2', 'tag3'],
    userSet: new Set(['user1', 'user2', 'user3']),
    configMap: new Map([['key1', 'value1'], ['key2', 'value2']]),
    calculate: function(x, y) {
        return x + y;
    },
    nested: {
        level: 1,
        value: 'nested value'
    },
    // Additional string properties
    stringProperty1: 'First string property',
    stringProperty2: 'Second string property',
    stringProperty3: 'Third string property',
    stringProperty4: 'Fourth string property',
    stringProperty5: 'Fifth string property',
    description: 'This is a comprehensive test object for heap dump property name testing',
    address: '123 Test Street, Test City, TC 12345',
    phoneNumber: '+1-555-123-4567',
    // Additional property types with self-explanatory names
    creationDate: new Date('2026-01-15'),
    version: '1.0.0',
    enabled: true,
    counter: 100,
    metadata: {
        author: 'Test Author',
        category: 'Testing',
        priority: 'High'
    }
};

globalThis.testObj = testObj;
