// https://github.com/NabuCasa/esp-web-flasher

azesp = {};

azesp.espStub = false;
azesp.ajax_status = 0;
azesp.download_data = {};


azesp.formatMacAddr = function(macAddr) {
  return macAddr
    .map((value) => value.toString(16).toUpperCase().padStart(2, "0"))
    .join(":");
};

// 接続
azesp.write_firm = async function(flash_list) {
    console.log("clickConnect: start");
    let esptoolMod = await import("./esptool/index.js");
    let esploader = await esptoolMod.connect({
        log: function(msg) { console.log(msg); },
        debug: function(msg) { console.log(msg); },
        error: function(msg) { console.log(msg); },
    });
    try {
        await esploader.initialize();

        console.log("Connected to " + esploader.chipName);
        console.log("MAC Address: " + azesp.formatMacAddr(esploader.macAddr()));

        espStub = await esploader.runStub();
        espStub.addEventListener("disconnect", function() {
            espStub = false;
        });
        // 転送速度設定
        await espStub.setBaudrate(115200);
        // 書込み
        let i;
        for (i in flash_list) {
            await azesp.write_data(flash_list[i]);
        }
        // 再起動
        await azesp.reboot();
        // 切断
        await esploader.disconnect();
    } catch (err) {
        await esploader.disconnect();
        console.error(err);
    }
    console.log("clickConnect: end");
};

azesp.reboot = async function () {
    // 再起動
    espStub.port.setSignals({ dataTerminalReady: false, requestToSend: true });
    await azesp.sleep(100);
    espStub.port.setSignals({ dataTerminalReady: false, requestToSend: false });
    await azesp.sleep(100);
};

// 指定したURLのファイルをダウンロードする
azesp.ajaxArrayBuffer = function(src) {
    var xhr = new XMLHttpRequest();
    xhr.open("GET", src, true);
    xhr.responseType = "arraybuffer";
    xhr.onload = function(e) {
        if (xhr.status == 200) {
            azesp.ajax_status = 2;
            azesp.download_data[src] = xhr.response;
        } else {
            azesp.ajax_status = 3;
            console.log("download status : " + xhr.status);
        }
    }
    xhr.send();
    azesp.ajax_status = 1;
};

// スリープ
azesp.sleep = function(ms) {
    return new Promise(function(resolve) { setTimeout(resolve, ms); });
};

// 配列をArrayBufferに変換する
azesp.arrayToArraybuffer = function(array_data) {
    var i;
    var r = new ArrayBuffer(array_data.length);
    var d = new DataView(r);
    for (i=0; i<array_data.length; i++) {
        d.setUint8(i, array_data[i]);
    }
    return r;
};

// データを書き込む
azesp.write_data = async function(flash_data) {
    let contents;
    // データ取得
    if (typeof flash_data.data == "string") {
          // 文字列ならばURLと判断してURLのファイルを取得してArrayBufferにする
          console.log("download start : " + flash_data.data);
          if (!azesp.download_data[flash_data.data]) azesp.ajaxArrayBuffer(flash_data.data);
          e = 0;
          while (true) {
              if (azesp.download_data[flash_data.data]) break;
              await azesp.sleep(200);
              if (azesp.ajax_status == 3) throw "download error : " + flash_data.data; // ajax失敗
              e++;
              if (e > 900) throw "download timeout : " + flash_data.data; // 3分待ってダウンロードできなければエラー
          }
          azesp.ajax_status = 0;
          console.log("download end : " + flash_data.data);
          contents = azesp.download_data[flash_data.data];
    } else if (Array.isArray(flash_data.data)) {
        contents = azesp.arrayToArraybuffer(flash_data.data); // 配列ならArrayBufferに変換
    } else {
        contents = flash_data.data; // それ以外はArrayBufferが入って来たと判定
    }
    // 書込み
    await espStub.flashData(
        contents,
        function (bytesWritten, totalBytes) {
            console.log("write: " + bytesWritten + " / " + totalBytes + " ("+ Math.floor((bytesWritten / totalBytes) * 100) +" %)");
        },
        flash_data.address
    );
    // ちょっと待つ
    await azesp.sleep(100);
};
