/*
 * Copyright (c) 2022 Huawei Device Co., Ltd.
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

declare function AssertType(value: any, type: string): void;

class Employee {
    protected name: string
    protected age: number

    constructor(name: string, age: number) {
        this.name = name;
        this.age = age;
    }

    getName(): string {
        AssertType(this, "Employee");
        AssertType(this.name, "string");
        return this.name;
    }

    getAge(): number {
        return this.age;
    }
}

class Manager extends Employee {
    private level: number;
    private static REQUIRED_LEVEL_MIN: number = 7;

    constructor(name: string, age: number, level: number) {
        super(name, age);
        this.level = level;
    }

    static getRequiredLevelMin(): number {
        AssertType(this, "typeof Manager");
        AssertType(this.REQUIRED_LEVEL_MIN, "number");
        return this.REQUIRED_LEVEL_MIN;
    }
}

function olderThan(this: Employee, age: number): boolean {
    AssertType(this, "Employee");
    AssertType(this.getAge(), "number");
    return this.getAge() > age;
}
