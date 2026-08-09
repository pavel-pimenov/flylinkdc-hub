# flylinkdc-hub

DC\+\+ hub [dc.fly-server.ru](http://dc.fly-server.ru)

## SAST Tools

[PVS-Studio](https://pvs-studio.com/pvs-studio/?utm_source=website&utm_medium=github&utm_campaign=open_source) - static analyzer for C, C\+\+, C#, and Java code.

### PVS-Studio Analysis Results

Полный отчёт: `pvs-studio-report-v5.txt` (84 сообщения).

| Уровень | Количество |
|---------|------------|
| err     | 4          |
| warn    | 0          |
| note    | 80         |
| **Итого** | **84**  |

**0 предупреждений (warn) в коде проекта.** Все оставшиеся `err` — V1065 (ложные срабатывания).

#### Оставшиеся сообщения (informational)

- **V1065** (4) — ложные срабатывания
- **V1042** (10) — copyleft лицензия в Lua-обёртках и skein (внешние библиотеки)
- **V525** (11) — похожие блоки кода в Lua API wrappers (шаблонный код)
- **V566** (10) — int→pointer casts в PXBReader (intentional `uintptr_t`)
- **V1048** (12) — повторные присвоения одного значения (init-паттерн)
- **V1027** (4) — `sockaddr_storage` → `sockaddr_in6/in` casts (BSD-стандарт)
- **V1029** (6) — усечение `size()`/`strlen()` в uint8/16 (ник/IP ≤ 255)