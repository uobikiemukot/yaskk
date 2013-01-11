# yaskk

yet another skk for terminal

# description
端末上で動作する簡易なskkです．

以下のような制限があります
-	全英・半角カナのモードは存在しない
-	辞書登録は今のところなし
-	出力はUTF-8のみ

<!--
# mode

## ASCII (MODE_ASCII)
-	入力されたものをスルーするだけ
-	Ctrl-J(LF)の場合のみ，かな入力モードへ

## ひらがな入力 (MODE_HIRA)
-	ローマ字入力ルールに従って変換
-	'l'でASCIIモードに戻る
-	'q'でカタカナモードへ

~~~
	あいうえおsh
	~~~~~~~~~~^^
	|         |
	|         変換中の文字列 (preedit)
	確定後の文字列
~~~

## カタカナ入力 (MODE_KATA)
-	ローマ字入力ルールに従って変換
-	'l'でASCIIモードに戻る
-	'q'でひらがなモードへ

## 変換 (MODE_COOK)
-	ひらがな・カタカナモード中にuppercaseを入力すると変換モードへ
-	送り仮名がない場合はSPACEで選択モードへ
-	送り仮名がある場合は送り仮名の開始文字をuppercaseで入力すると送り仮名の確定待ちへ

~~~
	▽あいうえおsh
	|~~~~~~~~~~^^
	|   |      |
	|   |      かな変換前の文字列 (preedit)
	|   かな変換後の文字列 (key)
    変換開始のマーク
~~~

## 確定待ち (MODE_APPEND)
-	

~~~
▽よみがえ*っt
|~~~~~~~~^^ ^
| |      || |
| |      || かな変換前の文字列 (preedit)
| |      |送り仮名 (append)
| |      確定待ちのマーク
| かな変換後の文字列 (key)
変換開始のマーク

~~~


## 選択 (MODE_SELECT)
-	SPACEで次候補
-	'x'で前候補

~~~
	▼愛
	| |
	| 変換候補
	選択中のマーク
~~~
-->

# 状態遷移

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
~~~

-	ひら・カタの状態でモードが変わるとpreeditは消える
-	ひら・カタの状態で"zl"が来たときのみ，ASCIIモードには遷移しない

~~~
      +-----------------------------+
      |         ひら or カタ        |
      +-----------------------------+
upper | ^ Ctrl+J                    ^ l (確定後にASCIIモードへ)
      | | Ctrl+H (▽を消す)          | q (確定後にひら・カタのトグル)
      | | q (ひら・カタ変換）       | Ctrl+J or その他の文字 (確定後に元のモードに戻る)
      | |                           | Ctrl+H (確定後，1文字消える)
      | |                           |
      v |         ESC               |
  +--------+ <--------------- +--------+
  |  変換  |                  |  選択  | Ctrl+P or x: 前候補
  +--------+ ---------------> +--------+ Ctrl+N or SPACE: 次候補
           |     SPACE        ^
     upper |                  | 送り仮名が確定すると自動で遷移
           v                  | (促音では遷移しない)
           +------------------+
           | 送り仮名確定待ち |
           +------------------+
             Ctrl+J: ひら or カタ モードへ戻る
             ESC: 変換モードへ戻る
~~~

## license
Copyright (c) 2012 haru (uobikiemukot at gmail dot com)

Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
