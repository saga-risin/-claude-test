# Resonance Frequency Detector

リアルタイムで共鳴周波数を検出するAudio Unit（AU）/ VST3プラグインです。

## 機能

- **FFTスペクトラムアナライザー** – 4096点FFT + Hann窓による高精度な周波数解析
- **共鳴ピーク検出** – 放物線補間付きローカル極大値検出で正確な周波数を推定
- **リアルタイム表示** – 30 FPSでスペクトラムとピークを更新
- **パラメータ**
  - Detection Threshold（-80dB ～ -20dB）
  - Max Peaks（1 ～ 8個）

## ビルド方法

### 必要なもの

- macOS 12以降（AUフォーマット）
- Xcode 14以降
- CMake 3.22以降
- Git

### ビルド手順

```bash
git clone <this-repo>
cd <repo-dir>
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release
```

初回ビルド時はJUCE（約400MB）が自動でダウンロードされます。

ビルド後、プラグインは自動的に以下へコピーされます。

- AU:   `~/Library/Audio/Plug-Ins/Components/Resonance Detector.component`
- VST3: `~/Library/Audio/Plug-Ins/VST3/Resonance Detector.vst3`

### DAWへの読み込み

Logic Pro / GarageBand でAUプラグインとして認識されます。プラグインが見つからない場合はAUスキャンを実行してください。

## アルゴリズム

1. 入力オーディオをモノラルミックスダウン
2. 4096サンプルのバッファが溜まるたびにFFT実行
3. Hann窓で時間窓をかけてスペクトル漏洩を低減
4. 指数移動平均（α=0.82）でスペクトルをスムージング
5. 閾値以上かつ両隣より大きいビンを極大値として検出
6. 放物線補間でサブビン精度の周波数推定
7. 100Hz以内の近接ピークを重複排除
8. 大きい順に最大N個を表示
