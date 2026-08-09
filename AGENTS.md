* Все изменения добавляй в файл HISTORY.md
* не анализируй каталоги sqlite prometheus-cpp и pugixml lua civetweb - это внешние либы их править не нужно.
* Анализируй код только для linux - win32 игнорируй
* Вноси все изменения не спрашива разрешений
* Поддержку сборки под Windows не сохраняй - проект только под линукс
* Используй clang-tidy
* После каждого изменения запускай тест `bash test-hub.sh` и убедись что ALL TESTS PASSED
* Правила наименования C++
  - Для членов класс используй в имени префикс m_
  - Для глобальных или статических перемых используй в имени префикс g_
  - Используй const где это необходимо.
* **Логирование критических ситуаций.** Везде, где хаб закрывает соединение из-за попытки сломать хаб или выхода за границы (атака, подозрительные данные, парсинг-ошибки, переполнение/недостаток данных, нарушение протокола) — ОБЯЗАТЕЛЬНО логируй причину ДО вызова `Close()`:
  1. `LogDbg`/`LogInfo`/`LogWarn` для system-лога (например: `LogWarn("[SECURITY] User %s (%s): MyINFO too long (%u bytes) - user closed.", ...)`)
  2. `UdpDebug::m_Ptr->BroadcastFormat("[SYS] ...")` для udp-дебага — только если уведомление оператору в чат уместно
  3. Никогда не оставляй голый `user->Close()` без пояснения причины в логах — по логам должно быть видно ЧТО и ПОЧЕМУ произошло, иначе невозможно диагностировать атаку или регрессию.
* **При добавлении новых ограничений/проверок на входные данные от клиентов** (размер, формат, диапазон) — всегда пиши в лог фактические значения (полученный размер, ник, IP, порт), чтобы по логам можно было понять какое именно значение триггернуло проверку.
* После каждого изменения метрик или добавления новых metric/prometheus переменных:
  1. Обнови скрипт генерации дашборда `grafana-dashboard.py` — добавь панели для новых метрик
  2. Запусти `python3 grafana-dashboard.py` для регенерации JSON
  3. Перезапусти Grafana: `docker compose restart grafana`
* Если меняется код хаба (core/*.cpp, core/*.h) — пересобери Docker образ: `docker compose build ptokax` и перезапусти: `docker compose up -d ptokax`
* После перезапуска контейнера **обязательно проверяй что хаб работает**: подожди 5 секунд после `docker compose up`, затем `docker compose logs ptokax --tail 30` — убедись что нет ошибок (TERMINATE, fatal, exception, segfault) и что видны подключения клиентов (PROTO, QUEUE, CMDS). Если хаб не стартует или есть ошибки — исправляй перед коммитом.
* Пакеты для сборки (Ubuntu 26.04) — см. `install-deps.sh`. Если обнаружен новый необходимый пакет — добавь его в `install-deps.sh` И в список выше.
  - build-essential cmake ninja-build mold ccache
  - liblua5.4-dev libsqlite3-dev
  - libcivetweb-dev libjsoncpp-dev prometheus-cpp-dev nlohmann-json3-dev libspdlog-dev
  - luacheck cppcheck
* После каждого коммита делай `git push`
* **После каждого коммита проверяй что хаб работает в контейнере:** подожди 5 секунд после `docker compose up`, затем `docker compose logs ptokax --tail 30` — убедись что нет ошибок (TERMINATE, fatal, exception, segfault) и что видны подключения клиентов (PROTO, QUEUE, CMDS). Если есть ошибки — исправляй перед следующим коммитом.
* **НЕ конвертируй файлы из CP1251 в UTF-8.** Хаб и клиент FlylinkDC++ общаются в кодировке CP1251. Lua-скрипты (scripts/), Settings.pxt, Motd.txt, language/Russian.xml, data-файлы — всё должно оставаться в CP1251. Если нужна работа с текстом — используй `iconv -f CP1251 -t UTF-8` для чтения, но пишите обратно в CP1251.
* **КРИТИЧНО: При редактировании Lua-файлов НИКОГДА не используй `write`/`edit` напрямую на CP1251 файлах.** Инструменты `write`/`edit` работают в UTF-8 и ПОВРЕЖДАЮТ CP1251 байты, заменяя их на UTF-8 replacement characters (U+FFFD = `EF BF BD`). Если нужно изменить CP1251 Lua-файл:
  1. `iconv -f CP1251 -t UTF-8 файл.lua > /tmp/file_utf8.lua`
  2. Правь `/tmp/file_utf8.lua` через edit/write
  3. `iconv -f UTF-8 -t CP1251 /tmp/file_utf8.lua > файл.lua`
  4. Проверь что не появилось `EF BF BD`: `xxd файл.lua | grep -c "efbfbd"` должно быть 0
* Тест `test-hub.sh` автоматически проверяет все Lua-файлы на наличие повреждённой CP1251 кодировки.
* Если заканчивается место на диске — запусти `bash clean-cache.sh`. Каталог `~/.cache/vscode-cpptools/ipch` занимает много места (precompiled headers). Также очищай Docker build cache: `docker builder prune -af`.
* **Делай изменения мелкими пачками и после каждого коммить с проверкой.** Не делай много изменений за один коммит. Каждое логическое изменение — отдельный коммит. После каждого коммита:
  1. Собери: `cmake --build out/build/default -j$(nproc)`
  2. Запусти тесты: `bash test-hub.sh` (убедись что ALL TESTS PASSED)
  3. Пересобери Docker: `docker compose build ptokax && docker compose up -d ptokax`
  4. Проверь логи: `sleep 5 && docker compose logs ptokax --tail 10` — убедись что нет TERMINATE/exception/segfault
  5. Закоммичь и запушь
  6. Только после этого делай следующее изменение
  Если после изменения хаб сломался — откати коммит (`git revert HEAD`) и не продолжай пока не разберёшься в причине.
* **НЕ ПУСКАЙ БИТЫЙ РЕЛИЗ В ПРОДУКТИВ.** Если хаб показывает мусор в чате, крашится, или клиенты получают ошибки — это битый релиз. Не коммичь и не пушь пока проблема не решена. Обязательно:
  1. Проверь что хаб работает после каждого изменения (логи без TERMINATE/exception)
  2. Проверь что клиенты не видят мусора в чате
  3. Если что-то не так — откати изменение и разберись
* **AddressSanitizer (ASan) включён по умолчанию в Debug-сборке.** Для запуска с ASan: `cmake --preset debug && cmake --build out/build/debug -j$(nproc)`. ASan ловит: use-after-free, buffer overflow, memory leaks, use-after-return. Проверяй логи ASan раз в день: `out/build/debug/logs/*.log` или stdout контейнера.
* **После каждого изменения (коммит, build, push) ВСЕГДА проверяй что хаб работает в контейнере.** Это не опционально — это обязательный шаг. Порядок:
  1. `docker compose build ptokax && docker compose up -d ptokax`
  2. `sleep 5 && docker compose logs ptokax --tail 30`
  3. Убедись: нет ошибок (TERMINATE, fatal, exception, segfault), видны подключения клиентов (PROTO, QUEUE, CMDS)
  4. Если хаб не стартует или есть ошибки — **НИКОГДА не продолжай**, откати и исправь
  Если тест-скрипт остановил контейнер — перезапусти его перед продолжением работы.
* **После запуска `test-hub.sh` проверяй что Docker-хаб восстановился.** Скрипт автоматически останавливает контейнер для тестирования и перезапускает его в cleanup. Если cleanup не удался — запусти вручную: `docker compose up -d ptokax && sleep 5 && docker compose logs ptokax --tail 10` и убедись что нет ошибок.