#!/usr/bin/env python3
# -*- coding: utf-8 -*-

"""
Copyright (c) 2024 Huawei Device Co., Ltd.
Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

    http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.

"""

import pytest
import os
import subprocess
import time

from selenium import webdriver
from selenium.webdriver.common.by import By
from selenium.webdriver.common.keys import Keys

driver = webdriver.Chrome()
root = None
file_input = None
script_directory = ''


def setup():
    global root
    os.chdir("..")
    subprocess.Popen(["python", 'dist/apDumpServer.py'])
    print("setup : Selenium opens the test page")
    time.sleep(2)
    driver.get("http://127.0.0.1:9001/ap/")
    application = driver.find_element(By.TAG_NAME, "ap-application")
    root = application.shadow_root.find_element(By.CSS_SELECTOR, ".root")


def test_import_file():
    global root, file_input, script_directory
    main_menu = root.find_element(By.TAG_NAME, "lit-main-menu")
    menu_body = main_menu.shadow_root.find_element(By.CSS_SELECTOR, ".menu-body")
    menu_group = menu_body.find_element(By.TAG_NAME, "lit-main-menu-group")
    menu_item = menu_group.find_element(By.TAG_NAME, "lit-main-menu-item")
    file_input = menu_item.shadow_root.find_element(By.CSS_SELECTOR, ".file")
    current_script_path = os.path.abspath(__file__)
    script_directory = os.path.dirname(current_script_path)
    path = os.path.join(script_directory, 'test.ap')
    file_path = os.path.normpath(path).replace("\\", "/")
    file_input.send_keys(file_path)
    time.sleep(2)


def test_node_click():
    global root
    app_content = root.find_element(By.CSS_SELECTOR, ".content")
    summary_div = app_content.find_element(By.TAG_NAME, "div")
    tab_summary = summary_div.find_element(By.TAG_NAME, "tab-ap-summary")
    summary_head = tab_summary.shadow_root.find_element(By.CSS_SELECTOR, ".tab-summary-head")
    expansion_div = summary_head.find_element(By.TAG_NAME, "div").find_element(By.TAG_NAME, "div")
    expansion_div.click()
    time.sleep(2)
    expansion_div.click()
    time.sleep(2)
    labels = summary_head.find_elements(By.TAG_NAME, "label")
    for label in labels:
        label.click()
        time.sleep(2)


def test_search():
    global root
    search_vessel = root.find_element(By.CSS_SELECTOR, ".search-vessel")
    search_div = search_vessel.find_element(By.CSS_SELECTOR, ".search")
    lit_search = search_div.find_element(By.CSS_SELECTOR, "lit-search")
    search_input = lit_search.shadow_root.find_element(By.CSS_SELECTOR, ".root").find_element(By.TAG_NAME, "input")
    search_input.send_keys("d")
    time.sleep(2)
    search_input.send_keys("w")
    time.sleep(2)
    current_text = search_input.get_attribute("value")
    for _ in range(len(current_text)):
        search_input.send_keys(Keys.BACK_SPACE)
    time.sleep(2)


def test_unsupported_file():
    global script_directory
    driver.refresh()
    application_node = driver.find_element(By.TAG_NAME, "ap-application")
    root_node = application_node.shadow_root.find_element(By.CSS_SELECTOR, ".root")
    main_menu = root_node.find_element(By.TAG_NAME, "lit-main-menu")
    menu_body = main_menu.shadow_root.find_element(By.CSS_SELECTOR, ".menu-body")
    menu_group = menu_body.find_element(By.TAG_NAME, "lit-main-menu-group")
    menu_item = menu_group.find_element(By.TAG_NAME, "lit-main-menu-item")
    file_input2 = menu_item.shadow_root.find_element(By.CSS_SELECTOR, ".file")
    path = os.path.join(script_directory, 'test.txt')
    file_path = os.path.normpath(path).replace("\\", "/")
    file_input2.send_keys(file_path)
    time.sleep(5)


def teardown():
    print("teardown : Close selenium")
    driver.close()
