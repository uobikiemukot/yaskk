# yaskk

yet another skk for terminal

## description

simple skk (japanese input method)

## configure

how to build yaskk

+	change conf.h (dictionary path, keymap, mark, etc...)
+	just type "make"

## build dictionary

 $ cd tool/
 (edit roma2kana)
 $ ./dict_builder.sh M roma2kana > /path/to/dict # use SKK-JISYO.M

## usage

~~~
$ yaskk [command] [args]...
~~~

add these lines to your screenrc for yass

~~~
bind j exec .!. yass
bind k eval 'exec cat' kill redisplay
~~~

## mode

~~~
        +-----------+
        |   ASCII   |
        +-----------+
        ^ |         ^
      l | | Ctrl-J  | l
        | v         |
  +--------+     +--------+
  |ひらがな|<--->|カタカナ|
  +--------+  q  +--------+

※ ひら・カタの状態でモードが変わるとpreeditは消える
※ ひら・カタの状態で"zl"が来たときのみ，ASCIIモードには遷移しない
~~~


~~~
                      +--------------------------------+
                      |         ひら or カタ           |
                      +--------------------------------+
                upper | ^ Ctrl+J (途中経過が確定)      ^ l (確定後にASCIIモードへ)
                      | | Ctrl+H (▽を消す)             | q (確定後にひら・カタのトグル)
                      | | ESC or Ctrl+G (途中経過消失) | Ctrl+J or その他の文字 (確定後に元のモードに戻る)
                      | |                              | Ctrl+H (確定後，1文字消える)
                      | |                              |
                      v |         ESC or Ctrl+G        |
                   +--------+ <------------------ +--------+
q (ひら・カタ変換) |  変換  |                     |  選択  | Ctrl+P or x: 前候補
                   +--------+ ------------------> +--------+ Ctrl+N or SPACE: 次候補
                            |        SPACE        ^
                      upper |                     | 送り仮名が確定すると自動で遷移
                            v                     | (促音では遷移しない)
                            +---------------------+
                            |  送り仮名確定待ち   |
                            +---------------------+
                              Ctrl+J: ひら or カタ モードへ戻る
                              ESC: 変換モードへ戻る
~~~

## license
Copyright (c) 2012 haru (uobikiemukot at gmail dot com)

Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
