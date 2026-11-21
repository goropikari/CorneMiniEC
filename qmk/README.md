# Corne MiniEC v0.2 の Vial 対応ファームウェア

miniEC と同じ7種類のキーマップを収録しています。すべて Vial で編集できます。Corne MiniEC v0.2 の左右には同じ UF2 を書き込みます。

## ファームウェアの入手

[GitHub Releases](https://github.com/goropikari/CorneMiniEC/releases) の Assets で、配列名に対応する `goropikari_corne_miniec_<keymap>.uf2` を選びます。バージョンタグが push されると、[リリースワークフロー](../.github/workflows/release-firmware.yml) が全キーマップをビルドして、UF2 とビルド情報を添付します。

## ビルド

Docker または Podman を用意し、CorneMiniEC リポジトリのルートで実行します。

```sh
make build
```

`vial` キーマップだけをビルドする場合は、次を実行します。

```sh
make build KEYMAP=vial
```

UF2 の出力先は `qmk/build/goropikari_corne_miniec_<keymap>.uf2` です。

## 書き込み

1. TRRS ケーブルを外し、片側ずつ XIAO RP2040 の BOOT ボタンを押したまま USB でPCに接続します。部品面を手前にして USB-C 端子を上に向けると、右下の「B」が BOOT ボタンです。
2. 表示された `RPI-RP2` ドライブに UF2 をコピーします。両側に同じファイルを書き込みます。
3. 両側の USB を外してから TRRS ケーブルをつなぎ、左側の USB をPCに接続します。

## Vial での編集

[Vial](https://get.vial.today/) を開きます。ロック解除を求められたら、左半分の一番左上のキーと、左半分の3段目で一番右のキーを同時に押します。

基板固有のスキャン設定は [Corne MiniEC v0.2 のソース](keyboards/goropikari/corne_miniec/readme.md) を参照してください。
