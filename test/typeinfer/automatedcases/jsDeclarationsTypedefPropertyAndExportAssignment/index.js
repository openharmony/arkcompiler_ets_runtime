/*
* Copyright (c) Microsoft Corporation. All rights reserved.
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
*
* This file has been modified by Huawei to verify type inference by adding verification statements.
*/

// === tests/cases/conformance/jsdoc/declarations/index.js ===
declare function AssertType(value:any, type:string):void;
const {taskGroups, taskNameToGroup} = require('./module.js');
AssertType(taskGroups, "{ parseHTML: { id: "parseHTML"; label: string; }; styleLayout: { id: "styleLayout"; label: string; }; }");
AssertType(taskNameToGroup, "{ [string]: import("tests/cases/conformance/jsdoc/declarations/module").TaskGroup; }");
AssertType(require('./module.js'), "typeof import("tests/cases/conformance/jsdoc/declarations/module")");
AssertType(require, "any");
AssertType('./module.js', "string");

/** @typedef {import('./module.js').TaskGroup} TaskGroup */

/**
 * @typedef TaskNode
 * @prop {TaskNode[]} children
 * @prop {TaskNode|undefined} parent
 * @prop {TaskGroup} group
 */

/** @typedef {{timers: Map<string, TaskNode>}} PriorTaskData */
class MainThreadTasks {
    /**
     * @param {TaskGroup} x
     * @param {TaskNode} y
     */
    constructor(x, y){}
}

module.exports = MainThreadTasks;
AssertType(module.exports = MainThreadTasks, "typeof MainThreadTasks");
AssertType(module.exports, "typeof MainThreadTasks");
AssertType(MainThreadTasks, "typeof MainThreadTasks");


