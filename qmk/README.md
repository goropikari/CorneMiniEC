# Corne MiniEC v0.2 の Vial 対応ファームウェア

miniEC と同じ7種類のキーマップを収録しています。すべて Vial で編集できます。キーマップごとに左用と右用の UF2 を用意しています。左右を設定したあとは、どちらの USB 端子でも PC に接続できます。

## ファームウェアの入手

[GitHub Releases](https://github.com/goropikari/CorneMiniEC/releases) の Assets で、選んだキーマップの `goropikari_corne_miniec_<keymap>_left.uf2` と `goropikari_corne_miniec_<keymap>_right.uf2` を入手します。バージョンタグが push されると、[リリースワークフロー](../.github/workflows/release-firmware.yml) が全キーマップをビルドして、UF2 とビルド情報を添付します。

## ビルド

Docker または Podman を用意し、CorneMiniEC リポジトリのルートで実行します。

```sh
make build
```

`vial` キーマップだけをビルドする場合は、次を実行します。

```sh
make build KEYMAP=vial
```

UF2 は `qmk/build/` に出力されます。全キーマップでは14ファイル、単体ビルドでは左右の2ファイルが生成されます。

## 書き込み

1. TRRS ケーブルを外し、片側ずつ XIAO RP2040 の BOOT ボタンを押したまま USB でPCに接続します。部品面を手前にして USB-C 端子を上に向けると、右下の「B」が BOOT ボタンです。
2. 表示された `RPI-RP2` ドライブに UF2 をコピーします。左側には `_left.uf2`、右側には同じキーマップの `_right.uf2` を書き込みます。
3. 両側の USB を外してから TRRS ケーブルをつなぎ、使いたい側の USB を PC に接続します。各側は UF2 で指定された左右を保存し、PC に USB 接続された側が起動時にマスターになります。

## キャリブレーション

起動時に各キーの未押下時の値を測定するため、USB を接続するときはキーを離しておきます。使用中もキーを離したときの測定値を使って基準値を更新します。

## Vial での編集

[Vial](https://get.vial.today/) を開きます。ロック解除を求められたら、左半分の一番左上のキーと、左半分の3段目で一番右のキーを同時に押します。

Vial で編集した配列は、USB 接続した側に保存されます。左右の USB 端子を使い分ける場合は、同じ配列を使えるよう両側に設定してください。

基板固有のスキャン設定は [Corne MiniEC v0.2 のソース](keyboards/goropikari/corne_miniec/readme.md) を参照してください。
