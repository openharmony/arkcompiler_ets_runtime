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
import '../base-ui/menu/MainMenu.js';
import '../base-ui/icon/LitIcon.js';
import '../base-ui/progress-bar/ProgressBar.js';
import './component/search/Search.js';
import './component/ApWelcome.js';
import {applicationHtml,} from './ApApplicationPublicFunc.js';
import './component/sheet/TabPaneApSummary.js';
import {TabPaneApSummary} from './component/sheet/TabPaneApSummary.js';

export class ApApplication extends HTMLElement {
    constructor() {
        super();
        this.attachShadow({mode: 'open'}).innerHTML = this.initHtml();
        this.initElements();
    }

    set search(search) {
        if (search) {
            this.setAttribute('search', '');
        } else {
            this.removeAttribute('search');
        }
    }

    get search() {
        return this.hasAttribute('search');
    }

    initHtml() {
        return applicationHtml;
    }

    initElements() {
        this.rootEL = this.shadowRoot.querySelector('.root');
        this.headerDiv = this.shadowRoot.querySelector('.search-vessel');
        this.apWelcome = this.shadowRoot.querySelector('#ap-welcome');
        this.apTreeTable = this.shadowRoot.querySelector('#tabpane-summary');
        this.mainMenu = this.shadowRoot?.querySelector('#main-menu');
        this.menu = this.mainMenu.shadowRoot?.querySelector('.menu-button');
        this.progressEL = this.shadowRoot?.querySelector('.progress');
        this.litSearch = this.shadowRoot?.querySelector('#lit-search');
        this.initElementsAttr();
        this.initSearchEvents();
        this.resetMenus();
        this.initGlobalEvents();
        this.initSearchChangeEvents();
    }

    initElementsAttr() {
        this.mainMenu.setAttribute('main_menu', '1');
        this.childComponent = [
            this.apTreeTable,
            this.apWelcome,
        ];
    }

    openTraceFile(file) {
        if (file === null) {
            return;
        }
        this.litSearch.style.marginLeft = '0px';
        this.openFileInit();
        this.parseApFile(file);
    }

    initGlobalDropEvents() {
        let body = document.querySelector('body');
        body.addEventListener('drop', (event) => {
            event.preventDefault();
            event.stopPropagation();
            if (this.rootEL.classList.contains('filedrag')) {
                this.rootEL.classList.remove('filedrag');
            }
            if (event && event.dataTransfer && event.dataTransfer.items &&
                event.dataTransfer.items.length > 0) {
                let item = event.dataTransfer.items[0];
                const entry = item.webkitGetAsEntry();
                if (entry?.isFile) {
                    this.openTraceFile(item.getAsFile());
                } else if (entry?.isDirectory) {
                    this.litSearch.setPercent('This File is not supported!', -1);
                    this.progressEL.loading = false;
                    this.freshMenuDisable(false);
                }
            }
        }, false);
    }

    initGlobalEvents() {
        let body = document.querySelector('body');
        body.addEventListener('dragover', (event) => {
            event.preventDefault();
            event.stopPropagation();
            if (event && event.dataTransfer && event.dataTransfer.items.length > 0 &&
                event.dataTransfer.items[0].kind === 'file') {
                event.dataTransfer.dropEffect = 'copy';
                if (!this.rootEL.classList.contains('filedrag')) {
                    this.rootEL.classList.add('filedrag');
                }
            }
        }, false);
        body.addEventListener('dragleave', (event) => {
            event.stopPropagation();
            event.preventDefault();
            if (this.rootEL.classList.contains('filedrag')) {
                this.rootEL.classList.remove('filedrag');
            }
        }, false);
        this.initGlobalDropEvents();
    }

    parseApFile(file) {
        let fileName = file.name;
        let showFileName = fileName.lastIndexOf('.') === -1 ? fileName : fileName.substring(0, fileName.lastIndexOf('.'));
        this.litSearch.setPercent('', 1);
        let fileSizeInKB = (file.size / 1024).toFixed(1);
        document.title = `${showFileName} (${fileSizeInKB}KB)`;
        if (!fileName.endsWith('.ap')) {
            this.litSearch.setPercent('This File is not supported!', -1);
            this.progressEL.loading = false;
            this.freshMenuDisable(false);
        } else {
            try {
                this.uploadFile(file).then((value) => {
                    this.litSearch.setPercent('', 101);
                    this.litSearch.style.pointerEvents = 'auto';
                    this.progressEL.loading = false;
                    this.freshMenuDisable(false);
                    this.apTreeTable.data = TabPaneApSummary.TransformJson(value);
                });
            } catch (httpError) {
                this.litSearch.setPercent('Parse Filed Please Wait', -1);
                this.progressEL.loading = false;
                this.freshMenuDisable(false);
            }
        }
    }

    async uploadFile(file) {
        const formData = new FormData();
        formData.append('file', file, file.name);
        const response = await fetch(`${window.location.origin}/ap/upload`, {
            method: 'POST',
            body: formData,
        });
        if (!response.ok) {
            throw new Error(`file Upload Failed，status：${response.status}`);
        }
        return await response.text();
    }

    initSearchEvents() {
        this.litSearch.addEventListener('focus', () => {
        });
        this.litSearch.addEventListener('ap-change', () => {
        });
        this.initSearchChangeEvents();
    }

    resetMenus() {
        this.mainMenu.menus = [
            {
                collapsed: false,
                title: 'Navigation',
                second: false,
                icon: 'caret-down',
                describe: 'Import ap file',
                children: [
                    {
                        title: 'Import ap file',
                        icon: 'folder',
                        fileChoose: true,
                        fileHandler: (ev) => {
                            this.openTraceFile(ev.detail);
                        },
                    }
                ],
            }
        ];
    }

    openFileInit() {
        this.litSearch.clear();
        this.headerDiv.style.pointerEvents = 'none';
        this.resetMenus();
        this.freshMenuDisable(true);
        this.showContent(this.apTreeTable);
        this.progressEL.loading = true;
    }

    initSearchChangeEvents() {
        let timer;
        this.litSearch.valueChangeHandler = (value) => {
            this.litSearch.list = [];
            if (timer) {
                clearTimeout(timer);
            }
            timer = setTimeout(() => {
                this.litSearch.isClearValue = false;
                if (value.length > 0) {
                    this.apTreeTable.filterData(value.toString().toLowerCase());
                } else {
                    this.litSearch.list = [];
                    this.apTreeTable.filterData('');
                }
                this.apTreeTable.expansionAll();
            }, 500);
        };
    }

    showContent(showNode) {
        if (showNode === this.apTreeTable) {
            this.menu.style.pointerEvents = 'auto';
            this.search = true;
            this.litSearch.style.display = 'block';
        } else {
            this.menu.style.pointerEvents = 'none';
            this.search = this.litSearch.isLoading;
            if (!this.search) {
                this.litSearch.style.display = 'none';
            }
        }
        this.childComponent.forEach((node) => {
            if (node === showNode) {
                showNode.style.visibility = 'visible';
            } else {
                node.style.visibility = 'hidden';
            }
        });
    }


    freshMenuDisable(disable) {
        let menus;
        this.mainMenu.menus[0].children[0].disabled = disable;
        if (this.mainMenu.menus.length > 2) {
            (menus = this.mainMenu.menus) === null || menus === void 0 ? void 0 : menus[1].children.map((it) => (it.disabled = disable));
        }
    }
}

if (!customElements.get('ap-application')) {
    customElements.define('ap-application', ApApplication);
}
