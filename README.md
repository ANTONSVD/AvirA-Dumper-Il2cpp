# AvirA Dumper Il2cpp

Простой дампер для игр на Unity Il2Cpp. Достает из игры все классы, методы и поля.

Что получается на выходе:

- `Il2cppDump.cs`. Весь код игры в виде текста. Удобно искать что угодно.
- Папка `DummyDll`. Все DLL игры, открываются в dnSpy.

Дампер сам правит байт версии метадаты на 1F в памяти, но только для файлов
SCPSL. Файл игры при этом не меняется.

## Как пользоваться

Нужна Visual Studio 2022 с C++.

1. Запусти `build.bat`. Соберется `out\AvirA_Dumper.exe`.
2. Запусти `out\AvirA_Dumper.exe`. Пути к игре он найдет сам.
3. Результат появится в `out\Dump`.

Можно указать файлы вручную:

```bat
out\AvirA_Dumper.exe GameAssembly.dll global-metadata.dat -o out\Dump
```

## Примечание

Сделано по мотивам Il2CppDumper от Perfare. Весь код написан с нуля.
