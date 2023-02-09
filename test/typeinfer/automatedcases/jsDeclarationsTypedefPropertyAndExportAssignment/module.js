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

// === tests/cases/conformance/jsdoc/declarations/module.js ===
declare function AssertType(value:any, type:string):void;
/** @typedef {'parseHTML'|'styleLayout'} TaskGroupIds */

/**
 * @typedef TaskGroup
 * @property {TaskGroupIds} id
 * @property {string} label
 * @property {string[]} traceEventNames
 */

/**
 * @type {{[P in TaskGroupIds]: {id: P, label: string}}}
 */
const taskGroups = {
AssertType(taskGroups, "{ parseHTML: { id: "parseHTML"; label: string; }; styleLayout: { id: "styleLayout"; label: string; }; }");
AssertType({    parseHTML: {        id: 'parseHTML',        label: 'Parse HTML & CSS'    },    styleLayout: {        id: 'styleLayout',        label: 'Style & Layout'    },}, "{ parseHTML: { id: "parseHTML"; label: string; }; styleLayout: { id: "styleLayout"; label: string; }; }");

    parseHTML: {
AssertType(parseHTML, "{ id: "parseHTML"; label: string; }");
AssertType({        id: 'parseHTML',        label: 'Parse HTML & CSS'    }, "{ id: "parseHTML"; label: string; }");

        id: 'parseHTML',
AssertType(id, "string");
AssertType('parseHTML', "string");

        label: 'Parse HTML & CSS'
AssertType(label, "string");
AssertType('Parse HTML & CSS', "string");

    },
    styleLayout: {
AssertType(styleLayout, "{ id: "styleLayout"; label: string; }");
AssertType({        id: 'styleLayout',        label: 'Style & Layout'    }, "{ id: "styleLayout"; label: string; }");

        id: 'styleLayout',
AssertType(id, "string");
AssertType('styleLayout', "string");

        label: 'Style & Layout'
AssertType(label, "string");
AssertType('Style & Layout', "string");

    },
}

/** @type {Object<string, TaskGroup>} */
const taskNameToGroup = {};
AssertType(taskNameToGroup, "{ [string]: TaskGroup; }");
AssertType({}, "{}");

module.exports = {
AssertType(module.exports = {    taskGroups,    taskNameToGroup,}, "typeof module.exports");
AssertType(module.exports, "typeof module.exports");
AssertType({    taskGroups,    taskNameToGroup,}, "{ taskGroups: { parseHTML: { id: "parseHTML"; label: string; }; styleLayout: { id: "styleLayout"; label: string; }; }; taskNameToGroup: { [string]: TaskGroup; }; }");

    taskGroups,
AssertType(taskGroups, "{ parseHTML: { id: "parseHTML"; label: string; }; styleLayout: { id: "styleLayout"; label: string; }; }");

    taskNameToGroup,
AssertType(taskNameToGroup, "{ [string]: TaskGroup; }");

};

