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
import {SearchHtml} from './Search.html.js';

const LOCAL_STORAGE_SEARCH_KEY = 'search_key';

export class LitSearch extends HTMLElement {


    constructor() {
        super();
        this._index = 0;
        this._list = [];
        this._value = false;
        this.historyMaxCount = 100;
        this.searchList = [];
        this.searchELList = [];
        this.attachShadow({mode: 'open'}).innerHTML = this.initHtml();
        this.initElements();
    }

    get list() {
        return this._list;
    }

    set list(value) {
        this._list = value;
    }

    get index() {
        return this._index;
    }

    set index(value) {
        this._index = value;
    }

    get searchValue() {
        return this.search?.value ?? '';
    }

    get isLoading() {
        return this.hasAttribute('isLoading');
    }

    set isLoading(va) {
        if (va) {
            this.setAttribute('isLoading', '');
        } else {
            this.removeAttribute('isLoading');
            window.localStorage.setItem(LOCAL_STORAGE_SEARCH_KEY, '');
        }
    }

    set isClearValue(value) {
        this._value = value;
    }

    get isClearValue() {
        return this._value;
    }

    valueChangeHandler = () => {
    };

    setPercent(name = '', value) {
        let searchHide = this.shadowRoot.querySelector('.root');
        let searchIcon = this.shadowRoot.querySelector('#search-icon');
        if (this.hasAttribute('textRoll')) {
            this.removeAttribute('textRoll');
        }
        this.isLoading = false;
        if (value > 0 && value <= 100) {
            searchHide.style.display = 'flex';
            searchHide.style.backgroundColor = '#e3e3e3';
            searchIcon === null || searchIcon === void 0 ? void 0 : searchIcon.setAttribute('name', 'cloud-sync');
            this.search.setAttribute('placeholder', `${name}${value}%`);
            this.search.setAttribute('readonly', '');
            this.search.className = 'readonly';
            this.isLoading = true;
        } else if (value > 100) {
            searchHide.style.display = 'flex';
            searchHide.style.backgroundColor = '#fff';
            searchIcon?.setAttribute('name', 'search');
            this.search?.setAttribute('placeholder', 'search');
            this.search?.removeAttribute('readonly');
            this.search.className = 'write';
        } else if (value === -1) {
            searchHide.style.display = 'flex';
            searchHide.style.backgroundColor = '#e3e3e3';
            searchIcon === null || searchIcon === void 0 ? void 0 : searchIcon.setAttribute('name', 'cloud-sync');
            this.search.setAttribute('placeholder', `${name}`);
            this.search.setAttribute('readonly', '');
            this.search.className = 'readonly';
        } else {
            searchHide.style.display = 'none';
        }
    }

    clear() {
        this.search = this.shadowRoot.querySelector('input');
        this.search.value = '';
        this.list = [];
    }

    blur() {
        this.search?.blur();
    }

    updateSearchList(searchStr) {
        if (searchStr === null || searchStr.length === 0 || searchStr.trim().length === 0) {
            return;
        }
        let searchInfo = this.searchList.find((searchInfo) => searchInfo.searchContent === searchStr);
        if (searchInfo !== undefined) {
            let index = this.searchList.indexOf(searchInfo);
            this.searchList.splice(index, 1);
            this.searchList.unshift({searchContent: searchStr, useCount: 1});
        } else {
            this.searchList.unshift({searchContent: searchStr, useCount: 1});
        }
    }

    getSearchHistory() {
        let searchString = window.localStorage.getItem(LOCAL_STORAGE_SEARCH_KEY);
        if (searchString) {
            let searHistory = JSON.parse(searchString);
            if (Array.isArray(searHistory)) {
                this.searchList = searHistory;
                return searHistory;
            }
        }
        return [];
    }

    searchFocusListener() {
    }

    searchBlurListener() {
        this.dispatchEvent(new CustomEvent('blur', {
            detail: {
                value: this.search.value,
            },
        }));
    }

    searchKeyupListener(e) {
        if (e.code === 'Enter' || e.code === 'NumpadEnter') {
            this.updateSearchList(this.search.value);
        } else {
            this.updateSearchHistoryList(this.search.value);
            this.valueChangeHandler?.call(this, this.trimSideSpace(this.search.value));
        }
        e.stopPropagation();
    }

    trimSideSpace(str) {
        return str.replace(/(^\s*)|(\s*$)/g, '');
    }

    initElements() {
        this.search = this.shadowRoot.querySelector('input');
        this.searchHistoryListEL = this.shadowRoot.querySelector('.search-history-list');
        this.retargetIndex = this.shadowRoot.querySelector('input[name="retarge_index"]');
        this.search.addEventListener('focus', () => {
        });
        this.search.addEventListener('blur', () => {
            this.searchBlurListener();
        });
        this.search.addEventListener('change', () => {
            this.index = -1;
            this.retargetIndex.value = '';
        });
        this.search.addEventListener('keyup', (e) => {
            this.retargetIndex.value = '';
            this.index = -1;
            this.searchKeyupListener(e);
        });
        (this.shadowRoot?.querySelector('input[name="retarget_index"]'))?.addEventListener('keydown', (event) => {
            if (event.key === 'Enter') {
                event.stopPropagation();
            }
        });
    }

    initHtml() {
        return SearchHtml;
    }

    hideSearchHistoryList() {
        this.searchHistoryListEL.style.display = 'none';
        if (this.searchList.length > this.historyMaxCount) {
            this.searchList = this.searchList.slice(0, this.historyMaxCount);
        }
        if (this.searchList.length === 0) {
            return;
        }
        let historyStr = JSON.stringify(this.searchList);
        window.localStorage.setItem(LOCAL_STORAGE_SEARCH_KEY, historyStr);
        this.searchList = [];
        this.searchELList = [];
    }

    updateSearchHistoryList(searchValue) {
        const keyword = searchValue.toLowerCase();
        this.searchELList.forEach((item) => {
            if (item.textContent.toLowerCase().includes(keyword)) {
                item.style.display = 'block';
            } else {
                item.style.display = 'none';
            }
        });
    }
}

if (!customElements.get('lit-search')) {
    customElements.define('lit-search', LitSearch);
}
