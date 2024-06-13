/*
 * Copyright (C) 2024 Huawei Device Co., Ltd.
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
import {TabPaneApSummaryHtml} from './TabPaneApSummary.html.js';

export class TabPaneApSummary extends HTMLElement {
    static jsonData = null;
    constructor() {
        super();
        this.apDataSource = [];
        this.apData = [];
        this.apTreeNodes = [];
        this.expandedNodeList = new Set();
        this.selectTreeDepth = 0;
        fetch('/ap/config.json')
            .then(response => response.json())
            .then(data => {
                TabPaneApSummary.jsonData = data;
                this.expansionClickEvent = () => {
                    let apItem;
                    this.expandedNodeList.clear();
                    if (((apItem = this.expansionUpIcon) === null || apItem === void 0 ? void 0 : apItem.name) === 'down') {
                        this.selectTreeDepth = 0;
                        this.expansionUpIcon.name = 'up';
                        this.expansionDownIcon.name = 'down';
                    } else {
                        this.selectTreeDepth = TabPaneApSummary.jsonData.summaryTreeLevel.length;
                        this.expansionUpIcon.name = 'down';
                        this.expansionDownIcon.name = 'up';
                    }
                    this.refreshSelectDepth(this.apTreeNodes);
                    this.refreshRowNodeTable(true);
                };
                this.attachShadow({mode: 'open'}).innerHTML = this.initHtml();
                this.initElements();
            });
    }

    set data(apDatas) {
        this.apDataSource = [];
        this.expandedNodeList.clear();
        this.expansionUpIcon.name = 'up';
        this.expansionDownIcon.name = 'down';
        this.apSummaryTable.innerHTML = '';
        this.apDataSource = apDatas;
        this.apData = this.apDataSource;
        if (this.apData.length !== 0) {
            this.refreshRowNodeTable();
        }
    }

    filterData(filter) {
        const keysOfApBean = TabPaneApSummary.jsonData.filterKeys;
        if (filter === '') {
            this.apData = this.apDataSource;
        } else {
            this.apData = this.apDataSource.filter((item) => {
                return keysOfApBean.some(field => {
                    const value = item[field];
                    return value !== undefined && value !== null && value.toString().toLowerCase().includes(filter);
                });
            });
        }
        this.expandedNodeList.clear();
        this.expansionUpIcon.name = 'up';
        this.expansionDownIcon.name = 'down';
        this.apSummaryTable.innerHTML = '';
        this.refreshRowNodeTable();
    }

    initElements() {
        this.apSummaryTable = this.shadowRoot?.querySelector('#tab-summary');
        this.expansionDiv = this.shadowRoot?.querySelector('.expansion-div');
        this.expansionUpIcon = this.shadowRoot?.querySelector('.expansion-up-icon');
        this.expansionDownIcon = this.shadowRoot?.querySelector('.expansion-down-icon');
        let summaryTreeLevel = TabPaneApSummary.jsonData.summaryTreeLevel;
        this.shadowRoot?.querySelectorAll('.head-label').forEach((summaryTreeHead) => {
            summaryTreeHead.addEventListener('click', () => {
                this.selectTreeDepth = summaryTreeLevel.indexOf(summaryTreeHead.textContent);
                this.expandedNodeList.clear();
                this.refreshSelectDepth(this.apTreeNodes);
                this.refreshRowNodeTable(true);
            });
        });
        this.apSummaryTable.onscroll = () => {
            let apTreeTableEl = this.shadowRoot?.querySelector('.ap-tree-table');
            if (apTreeTableEl) {
                apTreeTableEl.scrollTop = (this.apSummaryTable?.scrollTop) || 0;
            }
        };
    }

    expansionAll() {
        this.selectTreeDepth = TabPaneApSummary.jsonData.summaryTreeLevel.length - 1;
        this.expandedNodeList.clear();
        this.refreshSelectDepth(this.apTreeNodes);
        this.refreshRowNodeTable(true);
    }

    initHtml() {
        return TabPaneApSummaryHtml;
    }

    connectedCallback() {
        new ResizeObserver(() => {
            this.parentElement.style.overflow = 'hidden';
            this.refreshRowNodeTable();
        }).observe(this.parentElement);
        this.expansionDiv?.addEventListener('click', this.expansionClickEvent);
    }

    disconnectedCallback() {
        this.expansionDiv?.removeEventListener('click', this.expansionClickEvent);
    }

    refreshSelectDepth(apTreeNodes) {
        apTreeNodes.forEach((item) => {
            if (item.depth < this.selectTreeDepth) {
                this.expandedNodeList.add(item.id);
                if (item.children.length > 0) {
                    this.refreshSelectDepth(item.children);
                }
            }
        });
    }

    createTr(rowNode, className, proper) {
        let elementTr = document.createElement('tr');
        let elementTd = document.createElement('td');
        elementTr.title = rowNode[proper];
        elementTd.textContent = rowNode[proper];
        elementTd.className = className;
        elementTr.appendChild(elementTd);
        return elementTr;
    }

    createRowNodeTableEL(rowNodeList, tableTreeEl, tableRightDivs) {
        let unitPadding = 20;
        let leftPadding = 5;
        rowNodeList.forEach((rowNode) => {
            let tableTreeRowEl = document.createElement('tr');
            tableTreeRowEl.className = 'tree-row-tr';
            tableTreeRowEl.title = `${rowNode.apName}`;
            let leftSpacingEl = document.createElement('td');
            leftSpacingEl.style.paddingLeft = `${rowNode.depth * unitPadding + leftPadding}px`;
            tableTreeRowEl.appendChild(leftSpacingEl);
            this.addToggleIconEl(rowNode, tableTreeRowEl);
            let rowNodeTextEL = document.createElement('td');
            rowNodeTextEL.textContent = `${rowNode.apName}`;
            rowNodeTextEL.className = 'row-name-td';
            tableTreeRowEl.appendChild(rowNodeTextEL);
            tableTreeEl.appendChild(tableTreeRowEl);
            let apTrs = TabPaneApSummary.jsonData.apTableTr;
            for (let index = 0; index < apTrs.length; index++) {
                let item = apTrs[index];
                Object.keys(item).forEach((key) => {
                    tableRightDivs[index].appendChild(this.createTr(rowNode, key, item[key]));
                });
            }
            if (rowNode.children && this.expandedNodeList.has(rowNode.id)) {
                this.createRowNodeTableEL(rowNode.children, tableTreeEl, tableRightDivs);
            }
        });
    }

    addToggleIconEl(rowNode, tableRowEl) {
        let toggleIconEl = document.createElement('td');
        let expandIcon = document.createElement('lit-icon');
        expandIcon.classList.add('tree-icon');
        if (rowNode.children && rowNode.children.length > 0) {
            toggleIconEl.appendChild(expandIcon);
            expandIcon.name = this.expandedNodeList.has(rowNode.id) ? 'minus-square' : 'plus-square';
            toggleIconEl.classList.add('expand-icon');
            toggleIconEl.addEventListener('click', () => {
                let scrollTop = this.apSummaryTable?.scrollTop ?? 0;
                this.changeNode(rowNode.id);
                this.apSummaryTable.scrollTop = scrollTop;
                let apTreeTableEl = this.shadowRoot?.querySelector('.ap-tree-table');
                if (apTreeTableEl) {
                    apTreeTableEl.scrollTop = scrollTop;
                }
            });
        }
        tableRowEl.appendChild(toggleIconEl);
    }

    changeNode(currentNode) {
        if (this.expandedNodeList.has(currentNode)) {
            this.expandedNodeList.delete(currentNode);
        } else {
            this.expandedNodeList.add(currentNode);
        }
        this.refreshRowNodeTable();
    }

    refreshRowNodeTable(useCacheRefresh = false) {
        if (this.apSummaryTable === undefined) {
            return;
        }
        this.apSummaryTable.innerHTML = '';
        if (this.apSummaryTable && this.parentElement) {
            this.apSummaryTable.style.height = `${this.parentElement.clientHeight - 30}px`;
        }
        if (!useCacheRefresh) {
            this.apTreeNodes = this.buildTreeTblNodes(this.apData);
        }
        let tableFragmentEl = document.createDocumentFragment();
        let tableTreeEl = document.createElement('div');
        tableTreeEl.className = 'ap-tree-table';
        let apTrs = TabPaneApSummary.jsonData.apTableTr;
        let tableRightDivs = [];
        for (let index = 0; index < apTrs.length; index++) {
            tableRightDivs.push(document.createElement('div'));
        }
        if (this.parentElement) {
            tableTreeEl.style.height = `${this.parentElement.clientHeight - 40}px`;
        }
        this.createRowNodeTableEL(this.apTreeNodes, tableTreeEl, tableRightDivs);
        let emptyTr = document.createElement('tr');
        emptyTr.className = 'tree-row-tr';
        tableTreeEl === null || tableTreeEl === void 0 ? void 0 : tableTreeEl.appendChild(emptyTr);
        tableFragmentEl.appendChild(tableTreeEl);
        for (let index = 0; index < tableRightDivs.length; index++) {
            let emptyItemTr = document.createElement('tr');
            emptyItemTr.className = 'tree-row-tr';
            tableRightDivs[index].appendChild(emptyItemTr);
            tableFragmentEl.appendChild(tableRightDivs[index]);
        }
        this.apSummaryTable.appendChild(tableFragmentEl);
    }

    buildTreeTblNodes(apTreeNodes) {
        let id = 0;
        let root = {
            id: id,
            depth: 0,
            children: [],
            apName: 'All',
            type: '',
            isRoot: '',
            apKind: '',
            apAbcId: '',
            apId: ''
        };
        apTreeNodes.forEach((item) => {
            id++;
            let moduleNode = root.children.find((node) => node.apName === item.moduleName);
            let result = this.buildItem(moduleNode, id, item, root);
            id = result.id;
            let offsetNode = result.offsetNode;
            if (offsetNode.depth === 2) {
                let typeNode;
                id++;
                typeNode = {
                    id: id, depth: 3, children: [], apName: item.type, type: '',
                    isRoot: `${item.isRoot}`,
                    apKind: item.apKind === '-' ? '-' : TabPaneApSummary.KindMap[item.apKind === undefined ?
                        'UnknowId' : item.apKind],
                    apAbcId: `${item.apAbcId}`,
                    apId: `${item.apId}`
                };
                offsetNode.children.push(typeNode);
            }
        });
        return root.children.sort((leftData, rightData) => {
            return leftData.apName.localeCompare(rightData.apName);
        });
    }

    buildItem(moduleNode, id, item, root) {
        if (!moduleNode) {
            id++;
            moduleNode = {
                id: id, depth: 0,
                children: [],
                apName: item.moduleName,
                type: '',
                isRoot: '-',
                apKind: '-',
                apAbcId: '-',
                apId: '-'
            };
            root.children.push(moduleNode);
        }
        let funcNode = moduleNode.children.find((node) => node.apName === item.funcName);
        if (!funcNode) {
            id++;
            funcNode = {
                id: id, depth: 1,
                children: [],
                apName: item.funcName,
                type: '',
                isRoot: '-',
                apKind: '-',
                apAbcId: '-',
                apId: '-'
            };
            moduleNode.children.push(funcNode);
        }
        if (item.offset === '-') {
            return {id: id, offsetNode: funcNode};
        }
        let offsetNode = funcNode.children.find((node) => node.apName === item.offset);
        if (!offsetNode) {
            id++;
            offsetNode = {
                id: id, depth: 2,
                children: [],
                apName: item.offset,
                type: '',
                isRoot: '-',
                apKind: '-',
                apAbcId: '-',
                apId: '-'
            };
            funcNode.children.push(offsetNode);
        }
        return {id, offsetNode};
    }

    static MakeApObj(moduleName, functionName, item) {
        if (item === undefined) {
            item = new TypeItem();
        }
        let abcId = this.abcFilePoolMap.get(String(item.abcId));
        if (abcId === undefined) {
            abcId = '-';
        }
        let transformedItem = {
            moduleName: moduleName,
            funcName: functionName,
            offset: (item === null || item === void 0 ? void 0 : item.typeOffset) === undefined ? '-' : item.typeOffset,
            type: (item === null || item === void 0 ? void 0 : item.typeName) === undefined ? '-' : item.typeName,
            apKind: (item === null || item === void 0 ? void 0 : item.kind) === undefined ? '-' : item.kind,
            apAbcId: (item === null || item === void 0 ? void 0 : item.abcId) === undefined ? '-' : abcId,
            apId: (item === null || item === void 0 ? void 0 : item.id) === undefined ? '-' : item.id,
            isRoot: (item === null || item === void 0 ? void 0 : item.isRoot) === undefined ? '-' : item.isRoot
        };
        return transformedItem;
    }

    static TransformJson(value) {
        let apRes = JSON.parse(value);
        let apData = JSON.parse(apRes.data);
        let jsonData = apData.recordDetail;
        let abcFilePool = apData.abcFilePool;
        this.abcFilePoolMap = new Map(abcFilePool.map(item => {
            let parts = item.abcFile.split(/\/|\\/);
            let fileName = parts.length > 1 ? parts[parts.length - 1] : item.abcFile;
            return [item.abcId, fileName];
        }));
        let result = [];
        for (let moduleItem of jsonData) {
            for (let functionItem of moduleItem.function) {
                if (functionItem.type.length === 0) {
                    result.push(TabPaneApSummary.MakeApObj(moduleItem.moduleName, functionItem.functionName));
                }
                for (let typeItem of functionItem.type) {
                    if (Array.isArray(typeItem)) {
                        for (let typesItem of typeItem) {
                            if (Array.isArray(typesItem)) {
                                for (let item of typesItem) {
                                    result.push(TabPaneApSummary.MakeApObj(moduleItem.moduleName, functionItem.functionName, item));
                                }
                            } else {
                                result.push(TabPaneApSummary.MakeApObj(moduleItem.moduleName, functionItem.functionName, typesItem));
                            }
                        }
                    } else {
                        result.push(TabPaneApSummary.MakeApObj(moduleItem.moduleName, functionItem.functionName, typeItem));
                    }
                }
            }
        }
        return result;
    }
}

TabPaneApSummary.KindMap = {
    '0': 'ClassId',
    '1': 'LiteralId',
    '2': 'BuiltinsId',
    '3': 'LegacyKind',
    '4': 'MethodId',
    '5': 'BuiltinFunctionId',
    '6': 'RecordClassId',
    '7': 'PrototypeId',
    '8': 'ConstructorId',
    '9': 'MegaStateKinds',
    '10': 'TotalKinds',
    '11': 'UnknowId',
    '12': 'GlobalsId'
};
TabPaneApSummary.abcFilePoolMap = new Map();
if (!customElements.get('tab-ap-summary')) {
    customElements.define('tab-ap-summary', TabPaneApSummary);
}
export class TypeItem {
    constructor() {
        this.typeName = '-';
        this.typeOffset = '-';
        this.isRoot = '-';
        this.kind = '-';
        this.abcId = '-';
        this.id = '-';
    }
}
