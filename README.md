# ArchiCAD Cassette Panel Add-on

ArchiCAD add-on для расчёта размеров кассет, планок и откосов на основе свойств выбранных окон и дверей, с записью результатов в параметры GDL объектов.

## Возможности

- **Расчёт размеров** кассет, планок и откосов на основе свойств окон/дверей
- **Запись результатов** в GDL объекты (параметры Text_3...Text_18)
- **Поддержка типов элементов**:
  - Тип 0: ОК-0, ДВ-0
  - Тип 1: ОК-1, ДВ-1
  - Тип 2: ОК-2, ДВ-2
- **Единый интерфейс** с отображением всех параметров и целевых объектов для всех типов
- **Импорт/экспорт CSV** с сохранением информации о типах элементов
- **Автоматическое определение типа** из ID элемента
- **Палитра настроек** с сохранением defaults в CSV

## Требования

- ArchiCAD 27
- API Development Kit 27.3001
- CMake 3.16+
- Visual Studio 2019+ (Windows)

## Быстрый старт

1. Соберите аддон (см. раздел "Сборка").
2. Подключите `CassettePanel_AC27.apx` в Archicad через Add-On Manager.
3. Откройте меню **Add-ons → Cassette Panel → Расчёт кассет**.
4. Для настройки defaults откройте **Add-ons → Cassette Panel → Настройки**.

## Сборка

См. [BUILD_INSTRUCTIONS.md](BUILD_INSTRUCTIONS.md). Минимальный набор команд:

```bash
cmake -S . -B build -DAC_API_DEVKIT_DIR="C:\Program Files\Graphisoft\API Development Kit 27.3001"
cmake --build build --config Release
```

Готовый аддон появится здесь:

```
ToArchicad/Release/CassettePanel_AC27.apx
```

## Установка аддона

1. Запустите Archicad.
2. Откройте **Options → Add-On Manager**.
3. Удалите старые версии Cassette Panel (если есть).
4. Добавьте файл:
   `ToArchicad/Release/CassettePanel_AC27.apx`
5. Перезапустите Archicad.

Важно: если подключено две версии аддона (например Debug и Release), в меню будут дублиты.

## Работа с палитрами

### Палитра "Расчёт кассет"

1. Откройте **Add-ons → Cassette Panel → Расчёт кассет**.
2. Нажмите **"Загрузить выделение"**.
3. Проверьте параметры (ширины, смещения, ID объектов).
4. Нажмите **"Рассчитать"**.
5. Нажмите **"Записать в объекты"** для записи результатов в GDL.
6. Для обновления defaults нажмите **"Обновить настройки"** в разделе "Параметры".

### Палитра "Настройки"

1. Откройте **Add-ons → Cassette Panel → Настройки**.
2. Измените значения.
3. Нажмите **"Сохранить"**.
4. Вернитесь в палитру расчёта и нажмите **"Обновить настройки"**.

## Настройки по умолчанию (CSV)

Файл настроек хранится здесь:

```
%APPDATA%\Graphisoft\CassettePanel\settings.csv
```

Формат: `key;value`. Пример:

```
key;value
defaultType;0
wallIdForFloorHeight;СН-МД1
showDuplicateWarning;1
type0.plankWidth;285
type0.slopeWidth;285
type0.offsetX;165
type0.offsetY;50
type0.plankId;OK-0_PLNK
type0.leftSlopeId;OK-0_LOTK
type0.rightSlopeId;OK-0_ROTK
type1_2.plankWidth;160
type1_2.slopeWidth;225
type1_2.offsetX;165
type1_2.offsetY;50
type1_2.x2Coeff;745
type1_2.cassetteId;OK-1_2_CASS
type1_2.plankId;OK-1_2_PLNK
type1_2.leftSlopeId;OK-1_2_LOTK
type1_2.rightSlopeId;OK-1_2_ROTK
```

Если файл открыт в Excel, запись может быть заблокирована.

## Импорт/экспорт CSV результатов

В палитре расчёта доступны кнопки **"Экспорт CSV"** и **"Импорт CSV"**.
Экспортирует рассчитанные кассеты/планки/откосы с типами элементов.

## Траблшутинг

**Не обновляются параметры в главной палитре:**
- Убедитесь, что подключён актуальный `.apx` из `ToArchicad/Release`.
- Нажмите кнопку **"Обновить настройки"** в разделе "Параметры".
- Проверьте, что `settings.csv` не пустой.

**В меню две записи "Кассеты":**
- Удалите старые версии аддона через Add-On Manager.

**`settings.csv` пустой:**
- Проверьте, что Archicad загружает новую версию аддона.
- Убедитесь, что файл не открыт в Excel и не стоит атрибут "только чтение".

## Структура проекта

- `Src/` - исходный код C++
- `RFIX/` - HTML палитры и ресурсы
- `RINT/` - ресурсы интерфейса
- `Plans/` - планы разработки
- `Translations/` - файлы переводов

## Лицензия

См. [LICENSE_SYSTEM.md](LICENSE_SYSTEM.md)
