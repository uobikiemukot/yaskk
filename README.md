# yaskk
yet another skk for terminal

# description
端末上で動作する簡易なskkです．

# configure

how to build yaskk

+	change conf.h (dictionary path or etc...)
+	just type "make"

add these lines to your screenrc for yass

~~~
bind j exec .!. yass
bind k eval 'exec cat' kill redisplay
~~~

example of yasf

~~~
$ echo -e "\nUmi gaMiEru\n" | yasf
海が見える
~~~

# usage

~~~
$ yaskk
~~~

~~~
$ yaskk [command] [args]...
~~~

# mode

## ASCII
-	Ctrl + J: ひらがなモードへ

## ひらがな・カタカナ
-	ESC or Ctrl + G: ASCIIモードへ
-	'l': ASCIIモードへ
-	'q': ひらがな・カタカナのトグル
-	uppercase: 変換モードへ

## 変換
-	Ctrl + J: 入力中の文字列を確定させる
-	ESC or Ctrl + G: 変換モードを抜ける
-	SPACE: 選択モードへ
-	Ctrl + L: ユーザ辞書の再読み込み
-	uppercase: 送り仮名モードへ

## 送り仮名
-	Ctrl + J: 入力中の文字列を確定させる(送り仮名は含まない)
-	ESC or Ctrl + G: 変換モードへ
-	uppercase: 無視(小文字として扱われる)

## 選択
-	Ctrl + P or 'x': 前候補へ
-	Ctrl + N or SPACE: 次候補へ
-	Ctrl + J: 選択中の候補を確定させる
-	ESC or Ctrl + G: 変換モードへ
-	'l': 選択中の候補を確定しASCIIモードへ
-	'q': 選択中の候補を確定しひらがな・カナカナのトグル(未実装)
-	others: 選択中の候補を確定し変換モードへ

# license
Copyright (c) 2012 haru (uobikiemukot at gmail dot com)

Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
