#!/bin/bash
# Форматирование кода через astyle
# Опции: Allman style, табуляция 4, конвертация табов, заполнение пустых строк

set -e

astyle \
    --indent=tab=4 \
    --style=allman \
    --convert-tabs \
    --fill-empty-lines \
    --max-continuation-indent=79 \
    --suffix=none \
    ./core/*.cpp \
    ./core/*.h \
    ./gui.win/*.cpp \
    ./gui.win/*.h
