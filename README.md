<h1 align="center">馃幉 Random Picker 路 闅忔満鎶界宸ュ叿</h1>

<p align="center">
  <em>浠庢湰鍦拌〃鏍间腑闅忔満鎶藉彇浜哄憳鍚嶅崟锛屾敮鎸?Excel / CSV锛岃嚜甯?GUI 鍥惧舰鐣岄潰</em>
</p>

<p align="center">
  <a href="https://github.com/Ni-ShuWu/random-picker/releases"><img src="https://img.shields.io/github/v/release/Ni-ShuWu/random-picker" alt="Release"></a>
  <a href="https://github.com/Ni-ShuWu/random-picker/releases"><img src="https://img.shields.io/github/downloads/Ni-ShuWu/random-picker/total" alt="Downloads"></a>
  <img src="https://img.shields.io/badge/C%2B%2B-17-blue" alt="C++17">
  <img src="https://img.shields.io/badge/Windows-10%2F11-brightgreen" alt="Windows">
</p>

---

**缁欒€佸笀/缁勭粐鑰呯殑宸ュ叿**锛氫粠 Excel 鎴?CSV 鍚嶅崟涓殢鏈烘娊鍙栨寚瀹氭暟閲忕殑浜猴紝骞跺睍绀洪檮鍔犱俊鎭紙瀛﹀彿銆佺彮绾х瓑锛夈€?
| GUI 妯″紡 | CLI 妯″紡 |
|---|---|
| 鍙屽嚮鍗崇敤锛屾墍瑙佸嵆鎵€寰?| 閫傚悎瀹氭椂/鎵归噺浠诲姟 |
| 娴忚閫夋嫨鏂囦欢 鈫?杈撳叆浜烘暟 鈫?涓€閿娊绛?| `--file list.xlsx --count 5` |

---

## 猬囷笍 涓嬭浇

<a href="https://github.com/Ni-ShuWu/random-picker/releases/download/v1.0.0/RandomPicker_Windows_v1.0.0.zip">
  馃摜 涓嬭浇 Windows 鐗?(1.6 MB)
</a>
<br>
<a href="https://github.com/Ni-ShuWu/random-picker/archive/refs/tags/v1.0.0.zip">
  馃摜 涓嬭浇婧愮爜
</a>

瑙ｅ帇 `RandomPicker_Windows_v1.0.0.zip` 鈫?鍙屽嚮 `鍚姩.bat` 鍗冲彲銆?
---

## 鉁?鍔熻兘

- **鏍煎紡鏀寔** 鈥?CSV / XLSX / TSV / TXT锛岃嚜鍔ㄨ瘑鍒?- **闅忔満鎶藉彇** 鈥?`std::mt19937_64` 楂橀殢鏈烘€э紝涓嶆斁鍥炴娊鏍?- **GUI 鍥惧舰鐣岄潰** 鈥?Win32 鍘熺敓锛屾棤闇€瀹夎浠讳綍妗嗘灦
- **CLI 鍛戒护琛?* 鈥?閫傚悎鑷姩鍖栬剼鏈€佸畾鏃朵换鍔?- **AI 澧炲己** 鈥?鍙€夎皟鐢?DeepSeek / OpenAI 鐢熸垚鎺ㄨ崘璇?- **缁撴灉瀵煎嚭** 鈥?鏂囨湰 / CSV / 鍓创鏉匡紝UTF-8 缂栫爜
- **绉嶅瓙澶嶇幇** 鈥?`--seed 42` 鍥哄畾绉嶅瓙锛屾娊绛剧粨鏋滃彲澶嶇幇

---

## 馃枼锔?鍥惧舰鐣岄潰

| 姝ラ | 璇存槑 |
|------|------|
| 鈶?閫夋枃浠?| 鐐瑰嚮銆屾祻瑙堚€︺€嶉€夋嫨 `.csv` 鎴?`.xlsx` |
| 鈶?濉汉鏁?| 杈撳叆瑕佹娊鍙栫殑鏁伴噺 |
| 鈶?鍙€夛細AI | 鍕鹃€夈€屽惎鐢?AI銆嶏紝濉叆 DeepSeek/OpenAI API Key |
| 鈶?寮€濮?| 鐐瑰嚮銆岎煄?寮€濮嬫娊绛俱€?|
| 鈶?瀵煎嚭 | 淇濆瓨涓?txt/csv锛屾垨澶嶅埗鍒板壀璐存澘 |

---

## 鈱笍 鍛戒护琛?
```shell
# 鍩烘湰鐢ㄦ硶
random_picker.exe --file students.xlsx --count 3

# 淇濆瓨缁撴灉
random_picker.exe --file list.csv --count 5 --output result.txt

# 鎸囧畾闅忔満绉嶅瓙锛堝鐜扮粨鏋滐級
random_picker.exe --file list.csv --count 3 --seed 42

# 鍚敤 AI 鎺ㄨ崘锛圖eepSeek 榛樿锛?random_picker.exe --file list.csv --count 3 --with-ai --api-key sk-xxx

# 鑷畾涔?AI 鎺ュ彛锛堝鎹㈠叾浠?API锛?random_picker.exe --file list.csv --count 3 --with-ai --api-key sk-xxx --api-baseurl https://api.openai.com
```

瀹屾暣鍙傛暟锛歚random_picker.exe --help`

---

## 馃搵 琛ㄦ牸鏍煎紡

绗竴琛屽繀椤绘槸琛ㄥご锛岃嚦灏戝寘鍚?`濮撳悕` 鍒楋紙涔熻瘑鍒?`name`銆乣xingming`銆乣鍚嶅瓧`銆乣student`銆乣瀛︾敓`锛夈€?
```csv
濮撳悕,瀛﹀彿,鐝骇,鎬у埆,澶囨敞
寮犱笁,2024001,璁＄畻鏈轰竴鐝?鐢?鐝
鏉庡洓,2024002,璁＄畻鏈轰竴鐝?鐢?
鐜嬩簲,2024003,璁＄畻鏈轰竴鐝?濂?
```

---

## 馃敡 浠庢簮鐮佹瀯寤?
**渚濊禆**锛欳Make 3.16+銆丟CC 14+ / MSVC 2022銆乴ibzip

```bash
# MSYS2 UCRT64 缁堢
pacman -S --needed mingw-w64-ucrt-x86_64-{gcc,cmake,ninja,libzip,zlib}
cmake -G Ninja -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build
# 鎴愬搧: build/random_picker.exe
```

Windows 涓€閿瀯寤猴細鍙屽嚮 `build.bat`

---

## 馃П 鎶€鏈爤

| 妯″潡 | 瀹炵幇 |
|------|------|
| 闅忔満鏁?| `std::random_device` + `std::mt19937_64` |
| CSV 瑙ｆ瀽 | 鎵嬪啓锛堟敮鎸佸紩鍙枫€佽浆涔夈€丅OM锛?|
| XLSX 瑙ｆ瀽 | libzip 瑙ｅ帇 + XML 瑙ｆ瀽锛坺ip/xml锛?|
| HTTP | WinHTTP锛堢郴缁熷師鐢燂級 |
| GUI | Win32 API锛圕ommon Controls 6锛?|
| AI | OpenAI / DeepSeek Chat Completions 鍗忚 |
| 鏃ュ織 | 鍐呯疆锛堢骇鍒帶鍒躲€佹枃浠惰緭鍑猴級 |

---

## 馃搫 License

MIT
