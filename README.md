# Hệ thống Giám sát Nồng độ và Tốc độ Truyền dịch thông minh (Smart Infusion Monitoring System)

Dự án thiết kế và xây dựng thiết bị IoT phục vụ giám sát y tế từ xa, tự động đo lường lưu lượng dịch truyền (giọt/phút và ml/h), giám sát khối lượng dung dịch còn lại trong chai truyền dịch và gửi cảnh báo thời gian thực lên hệ thống Cloud (ThingsBoard).

Hệ thống sử dụng vi điều khiển chính là **ESP32 NodeMCU DevKit V1**, kết hợp cảm biến ngắt hồng ngoại **ITR9606** và cảm biến trọng lượng **Loadcell 5kg + HX711**.

---

## 🌟 Tính năng nổi bật

* **Đo lường lưu lượng truyền dịch thời gian thực:**
  * Đếm tổng số giọt dịch truyền đi qua buồng nhỏ giọt.
  * Tự động tính toán tốc độ truyền theo đơn vị **DPM** (Drops Per Minute - số giọt/phút).
  * Quy đổi chính xác lưu lượng theo đơn vị thể tích **ml/h** (mili-lít/giờ) dựa trên hệ số giọt tiêu chuẩn ($20\text{ giọt} = 1\text{ ml}$).
* **Giám sát khối lượng dịch truyền còn lại:**
  * Sử dụng Loadcell 5kg liên tục đo trọng lượng của chai dịch truyền (độ chính xác $0.1\text{g}$).
  * Hỗ trợ tự động trừ bì (Tare) khi khởi động thiết bị.
* **Hệ thống cảnh báo an toàn y tế chủ động (Alert System):**
  * Khi lượng dịch truyền trong chai còn dưới **`50g`** (ngưỡng tới hạn), còi báo động (Buzzer) trên mạch sẽ chủ động phát tiếng bíp ngắt quãng liên tục mỗi 3 giây.
  * Màn hình OLED chuyển sang trạng thái cảnh báo **`ALERT`** nhấp nháy liên tục để cảnh báo nhân viên y tế đến rút kim truyền kịp thời.
* **Đồng bộ hóa dữ liệu không dây (IoT Cloud):**
  * Kết nối Wi-Fi truyền nhận dữ liệu qua giao thức siêu nhẹ **MQTT** lên nền tảng **ThingsBoard Cloud**.
  * Cung cấp Dashboard giám sát từ xa theo thời gian thực (đồ thị đường lịch sử cân nặng, đồ thị lưu lượng dịch truyền, đồng bộ hóa đèn cảnh báo trạng thái).
* **Hoạt động không chặn (Non-blocking):**
  * Mã nguồn được thiết kế tối ưu, sử dụng ngắt cứng (External Interrupt) cho cảm biến đếm giọt và bộ hẹn giờ không chặn (`millis()`). Thiết bị luôn chạy mượt mà ngay cả khi kết nối Wi-Fi hoặc máy chủ Cloud bị gián đoạn.

---

## 🔌 Sơ đồ kết nối phần cứng (Pinout)

Thiết bị được thiết kế và gá đặt trên mạch in (PCB) với sơ đồ kết nối chân vi điều khiển ESP32 như sau:

| Thành phần | Tên chân trên mạch | Chân kết nối ESP32 | Mô tả chức năng |
| :--- | :--- | :--- | :--- |
| **Cảm biến hồng ngoại ITR9606** | `DROP_COUNT` | **GPIO18** | Nhận xung ngắt đếm giọt (khe chữ U kẹp ở bầu nhỏ giọt) |
| **Cảm biến trọng lượng HX711** | `HX711_DT` (Data) | **GPIO16 (RX2)** | Đường truyền dữ liệu cân nặng từ ADC 24-bit |
| **Cảm biến trọng lượng HX711** | `HX711_SCK` (Clock) | **GPIO17 (TX2)** | Xung nhịp đồng bộ của ADC 24-bit |
| **Màn hình OLED 128x64 I2C** | `OLED_SDA` | **GPIO21** | Đường truyền dữ liệu I2C |
| **Màn hình OLED 128x64 I2C** | `OLED_SCL` | **GPIO22** | Đường xung nhịp I2C |
| **Còi báo động (Buzzer)** | `BUZZER_CTRL` | **GPIO19** | Điều khiển còi chủ động (Active Buzzer) qua transistor C1815 |

---

## 🛠️ Yêu cầu phần mềm & Thư viện sử dụng

Để biên dịch mã nguồn, cần chuẩn bị môi trường **Arduino IDE** (hoặc Arduino CLI) hỗ trợ dòng chip ESP32 và cài đặt các thư viện sau:
* **Adafruit SSD1306** (hỗ trợ màn hình OLED)
* **Adafruit GFX Library** (thư viện đồ họa cốt lõi)
* **HX711** (Thư viện giao tiếp cảm biến lực của tác giả *bogde*)
* **PubSubClient** (Hỗ trợ giao thức MQTT kết nối Cloud)

---

## 🚀 Hướng dẫn triển khai và Cấu hình nhanh

### 1. Cấu hình thông tin kết nối
Mở file [SmartInfusionMonitor.ino](file:///c:/Users/Public/Documents/Altium/2026/Nong_Do_Toc_Do_Truyen_Dich/SmartInfusionMonitor/SmartInfusionMonitor.ino) trong thư mục dự án và thay thế các thông tin kết nối của bạn:

```cpp
// 1. Thông tin Wi-Fi nhà bạn
const char* WIFI_SSID     = "TÊN_WIFI_CỦA_BẠN";
const char* WIFI_PASSWORD = "MẬT_KHẨU_WIFI";

// 2. Thông tin kết nối ThingsBoard Cloud
const char* TB_SERVER   = "mqtt.eu.thingsboard.cloud"; // Máy chủ MQTT ThingsBoard châu Âu
const int   TB_PORT     = 1883;
const char* TB_TOKEN    = "ACCESS_TOKEN_THIẾT_BỊ_CỦA_BẠN"; // Token lấy trên ThingsBoard
```

### 2. Biên dịch và nạp code
1. Cắm cáp USB kết nối ESP32 với máy tính.
2. Chọn Board là **`ESP32 Dev Module`** và đúng cổng COM tương ứng trên Arduino IDE.
3. Nhấn nút **`Upload`** để nạp code.

---

## 📐 Thiết kế phần cứng (Altium Designer)

Dự án đi kèm thư mục [PCB_Project/](file:///c:/Users/Public/Documents/Altium/2026/Nong_Do_Toc_Do_Truyen_Dich/PCB_Project) chứa toàn bộ thiết kế phần cứng chuyên nghiệp bằng phần mềm **Altium Designer**:
* **`Sheet1.SchDoc`**: Mạch nguyên lý chi tiết hệ thống (bao gồm khối nguồn hạ áp, khối điều khiển ESP32, khối cảm biến và còi báo transistor).
* **`PCB1.PcbDoc`**: Thiết kế mạch in 2 lớp, đi dây tối ưu hóa chống nhiễu cho tín hiệu analog nhỏ từ HX711 và vùng Keepout ăng-ten phát sóng RF của ESP32.
* **`Truyen_dich.PrjPcb`**: File quản lý dự án phần cứng trên Altium.

---

## 📈 Dashboard giám sát ThingsBoard

Giao diện giám sát từ xa được thiết kế sinh động bao gồm:
* Đồng hồ đo thể tích dạng kim (Radial Gauge) hiển thị trực quan lượng dịch truyền còn lại theo gram.
* Biểu đồ thời gian thực (Timeseries Chart) vẽ đường dốc đi xuống của cân nặng chai dịch truyền và đường đi ngang ổn định của tốc độ giọt (DPM).
* Đèn trạng thái khẩn cấp (State indicator): Màu xanh lá cây biểu thị an toàn (`SAFE`), tự động nhấp nháy đỏ liên tục khi phát hiện trạng thái báo động hết dịch (`ALERT`).
