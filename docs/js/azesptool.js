// https://github.com/NabuCasa/esp-web-flasher


azesp = {};

azesp.serialLib = !navigator.serial && navigator.usb ? serial : navigator.serial;
azesp.esptoolMod = import("./esptool/bundle.js");


azesp.espStub = false;
azesp.ajax_status = 0;
azesp.download_data = {};

azesp.info_div = "";

// 書き込みのターミナル用
azesp.espLoaderTerminal = {
    "clean": function() {
        azesp.log("");
    },
    "writeLine": function(data) {
        azesp.log(data);
    },
    "write": function(data) {
        azesp.log(data);
    }
};

azesp.log = function(msg) {
    console.log(msg);
    if (azesp.info_div) {
        $("#"+azesp.info_div).html(msg);
    }
};

azesp.formatMacAddr = function(macAddr) {
  return macAddr
    .map((value) => value.toString(16).toUpperCase().padStart(2, "0"))
    .join(":");
};

// フラッシュ内クリア
azesp.erase_flash = async function(write_speed, info_id) {
    let baudrate = (write_speed)? write_speed: 115200;
    if (info_id) azesp.info_div = info_id;
    try {
        let esploader = await azesp.esptool.ESPLoader.connect({
            log: azesp.log,
            debug: azesp.log,
            error: azesp.log,
        });
        await esploader.initialize();

        azesp.log("Connected to " + esploader.chipName);
        azesp.log("MAC Address: " + azesp.formatMacAddr(esploader.macAddr()));

        espStub = await esploader.runStub();
        espStub.addEventListener("disconnect", function() {
            espStub = false;
        });
        // 転送速度設定
        await espStub.setBaudrate(baudrate);
        // 削除
        azesp.log("Erase Started.");
        await azesp.sleep(1000);
        await espStub.eraseFlash();
        await azesp.sleep(1000);
        // 再起動
        azesp.log("Reboot Started.");
        await azesp.reboot();
        await azesp.sleep(1000);
        // 切断
        await esploader.disconnect();
        await azesp.sleep(1000);
        if (espStub && espStub.port) espStub.port.close();
        azesp.log("Erase Complated.");

    } catch (err) {
        azesp.log("Error : " + err);
    }
};

// 書込み
azesp.write_firm = async function(flash_list, write_speed, info_id) {
    let baudrate = (write_speed)? write_speed: 115200;
    if (info_id) azesp.info_div = info_id;
    
    // 接続
    azesp.esptool = await azesp.esptoolMod;
    azesp.device = await azesp.serialLib.requestPort({}); // シリアルポートに接続
    azesp.transport = new azesp.esptool.Transport(azesp.device, true); // シリアルポートに接続
    azesp.esploader = new azesp.esptool.ESPLoader({ // ESPTOOL 初期化
        "transport": azesp.transport,
        "baudrate": parseInt(baudrate),
        "terminal": azesp.espLoaderTerminal,
        "debugLogging": true
    });

    azesp.chip = await azesp.esploader.main(); // 
    azesp.log("Connected to " + azesp.chip);
    azesp.log("MAC Address: " + azesp.esploader.chip.UART_DATE_REG_ADDR);

    // データ書き込み
    let i, write_data_list = [];
    for (i in flash_list) {
        write_data_list.push({
            "address": flash_list[i].address,
            "data": await azesp.load_data(flash_list[i])
        });
    }
    console.log(write_data_list);
    // 書込み
    await azesp.esploader.writeFlash({
        "fileArray": write_data_list,
        "flashSize": "keep",
        "eraseAll": false,
        "compress": true,
        "reportProgress": function(fileIndex, bytesWritten, totalBytes) {
            azesp.log("Write : " + bytesWritten + " / " + totalBytes + " ("+ Math.floor((bytesWritten / totalBytes) * 100) +" %)");
        },
        "calculateMD5Hash": function(image) {
            // CryptoJS.MD5(CryptoJS.enc.Latin1.parse(image));
        }
    });
    // ちょっと待つ
    await azesp.sleep(100);
    // 後処理
    await azesp.esploader.after();

    // 切断
    await azesp.transport.disconnect();

    // ESP32 再起動
    azesp.log("Reboot Started.");
    await azesp.reboot();
    await azesp.sleep(1000);

    azesp.log("Write Complated.");

    return;
        try {


        let esploader = await esptoolMod.ESPLoader.prototype.connect(baudrate, {
            log: azesp.log,
            debug: azesp.log,
            error: azesp.log,
        });
        await esploader.initialize();

        azesp.log("Connected to " + esploader.chipName);
        azesp.log("MAC Address: " + azesp.formatMacAddr(esploader.macAddr()));

        espStub = await esploader.runStub();
        espStub.addEventListener("disconnect", function() {
            espStub = false;
        });
        // 転送速度設定
        await espStub.setBaudrate(baudrate);
        // 書込み
        let i;
        for (i in flash_list) {
            await azesp.write_data(flash_list[i]);
        }
        await azesp.sleep(1000);
        // 再起動
        azesp.log("Reboot Started.");
        await azesp.reboot();
        await azesp.sleep(1000);
        // 切断
        await esploader.disconnect();
        await azesp.sleep(1000);
        if (espStub && espStub.port) espStub.port.close();
        azesp.log("Write Complated.");
    } catch (err) {
        azesp.log("Error : " + err);
        if (esploader) await esploader.disconnect();
    }
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
            azesp.log("Download Status : " + xhr.status);
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
azesp.load_data = async function(flash_data) {
    let contents;
    // データ取得
    if (typeof flash_data.data == "string") {
          // 文字列ならばURLと判断してURLのファイルを取得してArrayBufferにする
          azesp.log("Download Start : " + flash_data.data);
          if (!azesp.download_data[flash_data.data]) azesp.ajaxArrayBuffer(flash_data.data);
          e = 0;
          while (true) {
              if (azesp.download_data[flash_data.data]) break;
              await azesp.sleep(200);
              if (azesp.ajax_status == 3) throw "Download Error : " + flash_data.data; // ajax失敗
              e++;
              if (e > 900) throw "Download Timeout : " + flash_data.data; // 3分待ってダウンロードできなければエラー
          }
          azesp.ajax_status = 0;
          azesp.log("Download Complated : " + flash_data.data);
          contents = azesp.download_data[flash_data.data];
    } else if (Array.isArray(flash_data.data)) {
        contents = azesp.arrayToArraybuffer(flash_data.data); // 配列ならArrayBufferに変換
    } else {
        contents = flash_data.data; // それ以外はArrayBufferが入って来たと判定
    }
    return contents;
};
