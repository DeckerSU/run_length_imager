Что это?
--------

Это проект основанный на исходниках rlimager.c от @makers_mark с форума http://forum.xda-developers.com/ для преобразования RLE RAW Encoded изображений просто к RAW.

Что нужно знать или полезные ссылки
-----------------------------------

- http://www.decker.su/2016/10/how-to-edit-splash-screen-on-qualcomm-devices.html - Редактируем Splash Screen в смартфонах на Qualcomm.
- https://source.codeaurora.org/quic/la/device/qcom/common/tree/display/logo?h=LA.BR.1.2.7_rb1.1 - оригинальный скрипт logo_gen.py для создания SPLASH раздела.
- https://habrahabr.ru/post/141827/ - Простейшие алгоритмы сжатия: RLE и LZ77 (теория)
- http://forum.xda-developers.com/android/software-hacking/guide-how-to-create-custom-boot-logo-t3470473 - [GUIDE] How to Create Custom Boot Logo (splash.img) for Snapdragon Devices
- http://forum.xda-developers.com/showthread.php?t=2764354 - [GUIDE][TOOL][v1.2]-=Solved=-The Google Splash Sceen & Bootloader Graphics
- https://github.com/balika011/android_device_qcom_splashtool

Что добавлено?
--------------

Как выяснилось, при попытке декодировать RLE-закодированное RGB24 RAW изображение из раздела SPLASH от Alcatel Idol 3 6045 и Alcatel Idol 4 6055 изначальный вариант утилиты неверно декодировал изображение, т.е. при попытке просмотреть его как RAW RGB24 любой просмотрщик выдавал "кашу" вместо картинки. Тогда я решил начитаться теории (особенно по ссылке на Хабре) и понял что **rlimager** изначально неверно декодирует картинку. Т.е. в оригинале он читал 4 байта, первый байт считал как количество повторений, а оставшиеся 3 - RGB-цвет (все это в функции decode-rgb24-rle), хотя в реализации от Alcatel это немного не так. Первый байт в старшем бите хранит флаг повторяемая эта последовательность или нет, остальные 7-бит длина. При этом если последовательность повторяемая, т.е. если старший бит байта длины установлен в 1, то мы повторяем последовательность (длина+1) раз. Если же старший бит равен нулю, то далее идет (длина+1) "одиночных" байт (вернее 3-х байт RGB цвета). Чтобы было понятно на примере RLE-кодированная последовательность:

``` 
 00000000:  8A C1 DE 00-01 01 02 03-04 05 06 FF-AA AA AA FF                                                                                                                  
```

Должна разворачиваться во что-то вроде:

```
 00000000:  00 DE C1 00-DE C1 00 DE-C1 00 DE C1-00 DE C1 00                                                                                                                  
 00000010:  DE C1 00 DE-C1 00 DE C1-00 DE C1 00-DE C1 00 DE                                                                                                                  
 00000020:  C1 03 02 01-06 05 04 AA-AA AA AA AA-AA AA AA AA                                                                                                                  
 00000030:  AA AA AA AA-AA AA AA AA-AA AA AA AA-AA AA AA AA                                                                                                                  
 00000040:  AA AA AA AA-AA AA AA AA-AA AA AA AA-AA AA AA AA                                                                                                                  
 00000050:  AA AA AA AA-AA AA AA AA-AA AA AA AA-AA AA AA AA                                                                                                                  
 00000060:  AA AA AA AA-AA AA AA AA-AA AA AA AA-AA AA AA AA                                                                                                                  
 00000070:  AA AA AA AA-AA AA AA AA-AA AA AA AA-AA AA AA AA                                                                                                                  
 00000080:  AA AA AA AA-AA AA AA AA-AA AA AA AA-AA AA AA AA                                                                                                                  
 00000090:  AA AA AA AA-AA AA AA AA-AA AA AA AA-AA AA AA AA                                                                                                                  
 000000A0:  AA AA AA AA-AA AA AA AA-AA AA AA AA-AA AA AA AA                                                                                                                  
 000000B0:  AA AA AA AA-AA AA AA AA-AA AA AA AA-AA AA AA AA                                                                                                                  
 000000C0:  AA AA AA AA-AA AA AA AA-AA AA AA AA-AA AA AA AA                                                                                                                  
 000000D0:  AA AA AA AA-AA AA AA AA-AA AA AA AA-AA AA AA AA                                                                                                                  
 000000E0:  AA AA AA AA-AA AA AA AA-AA AA AA AA-AA AA AA AA                                                                                                                  
 000000F0:  AA AA AA AA-AA AA AA AA-AA AA AA AA-AA AA AA AA                                                                                                                  
 00000100:  AA AA AA AA-AA AA AA AA-AA AA AA AA-AA AA AA AA                                                                                                                  
 00000110:  AA AA AA AA-AA AA AA AA-AA AA AA AA-AA AA AA AA                                                                                                                  
 00000120:  AA AA AA AA-AA AA AA AA-AA AA AA AA-AA AA AA AA                                                                                                                  
 00000130:  AA AA AA AA-AA AA AA AA-AA AA AA AA-AA AA AA AA                                                                                                                  
 00000140:  AA AA AA AA-AA AA AA AA-AA AA AA AA-AA AA AA AA                                                                                                                  
 00000150:  AA AA AA AA-AA AA AA AA-AA AA AA AA-AA AA AA AA                                                                                                                  
 00000160:  AA AA AA AA-AA AA AA AA-AA AA AA AA-AA AA AA AA                                                                                                                  
 00000170:  AA AA AA AA-AA AA AA AA-AA AA AA AA-AA AA AA AA                                                                                                                  
 00000180:  AA AA AA AA-AA AA AA AA-AA AA AA AA-AA AA AA AA                                                                                                                  
 00000190:  AA AA AA AA-AA AA AA AA-AA AA AA AA-AA AA AA AA                                                                                                                  
 000001A0:  AA AA AA AA-AA AA AA 00-00 00 00 00-00 00 00 00                                                                                                                  
 000001B0:  00 00 00 00-00 00 00 00-00 00 00 00-00 00 00 00                                                                                                                  
 000001C0:  00 00 00 00-00 00 00 00-00 00 00 00-00 00 00 00                                                                                                                  
 000001D0:  00 00 00 00-00 00 00 00-00 00 00 00-00 00 00 00                                                                                                                  
                                                                                                                                   
```

RGB и BGR здесь правда поменяны местами, но суть от этого не меняется. Т.е. самый первый у нас идет байт 0x8A, это означает что последовательность повторяемая (старший бит установлен в 1) и повторить R,G,B = 0xDE,0xC1,0x00 нужно ровно 11 (0xB) раз. Что мы и видим в декодированном варианте. Далее у нас идет байт, который сигнализирует нам о том что повторения не будет (старший бит установлен в 0), но количество элементов (длина) = 2 (в байте единица, ну а длина, т.е. количество элементов = длина + 1 = 2). Читаем два раза подряд по 3 байта, т.е. точки RGB и просто повторяем их. Ну а далее у нас 128 групп из R,G,B = 0xAA, 0xAA, 0xAA. Что мы и видим. Вот на пальцах и вся модификация, которую я внес в коммите с названием Decode input_file from RLE encoded RAW BGR24 format.

Теперь у программы появился ключик -d 5 для декодирования такого формата.

Что еще полезного есть в этом репозитории?
------------------------------------------

Добавлены несколько скриптов на Python'е для кодирования / декодирования RLE. Принцип как оно работает понятен из текста. code.py из RAW делает RLE Encoded, а decode.py из RLE Encoded делает RAW. Обратите внимание, что речь идет про работу с RGB24 RAW изображениями. Т.е. цвет каждого пикселя у вас кодируется тремя (!) байтами. В скриптах на Python'е при этом важны размеры изображения, по-умолчанию там установлено 1080x1920.
