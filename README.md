# AvirA Dumper Il2cpp

Статический дампер для игр на Unity Il2Cpp. Достает из игры все классы, методы и поля.

Что получается на выходе:

- `Il2cppDump.cs` — весь код игры в виде C# текста. По нему удобно все искать.
- `script.json` — RVA всех методов, строковых слотов и метадаты. Для IDA/Ghidra.
- `stringliteral.json` — все строковые литералы игры.
- `il2cpp.h` — структуры всех классов в C, для IDA/Ghidra.
- `ida.py`, `ida_with_struct.py` — скрипты для IDA, второй еще и типы применяет.
- `ghidra.py` — скрипт для Ghidra.
- Папка `DummyDll` — все DLL игры, открываются в dnSpy.

Дампер сам правит байт версии метадаты на 1F в памяти, но только для файлов
SCPSL. Файл игры при этом не меняется.

## Как пользоваться

Нужна Visual Studio 2022 с C++.

1. Открой `AvirADumper.sln` в Visual Studio и нажми Build. Соберется `out\AvirADumper.exe`.
2. Запусти `out\AvirADumper.exe`. Он сам найдет установленные игры на Unity.
Если игра одна, дамп начнется сразу. Если несколько, выбери номер из списка.
3. Результат появится в `out\Dump_НазваниеИгры`.

Можно собрать из консоли:

```bat
msbuild AvirADumper.vcxproj /p:Configuration=Release /p:Platform=x64
```

Можно указать файлы вручную:

```bat
out\AvirADumper.exe GameAssembly.dll global-metadata.dat -o out\Dump
```

## Скрипты для IDA и Ghidra

Адреса в `script.json` это RVA относительно базы `GameAssembly.dll`.

**IDA**: File → Script file → выбери `ida.py` (или `ida_with_struct.py`),
затем укажи `script.json` (и `il2cpp.h` для второго). Скрипт создаст функции
и переименует все методы, строки и метадату.

**Ghidra**: Window → Script Manager → Add script directory (папка дампа),
запусти `ghidra.py` и укажи `script.json`.

## Примечание

Сделано по мотивам Il2CppDumper от Perfare. Весь код написан с нуля.
