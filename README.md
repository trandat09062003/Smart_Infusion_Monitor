# Hệ thống Giám sát Lưu lượng và Khối lượng Chai Dịch truyền Y tế (IoT)

Dự án nghiên cứu và thiết kế thiết bị IoT giám sát dịch truyền y tế từ xa phục vụ bệnh viện/phòng khám. Hệ thống tự động đo tốc độ nhỏ giọt (DPM), tính lưu lượng thể tích (ml/h), giám sát khối lượng dung dịch còn lại trong chai truyền và phát cảnh báo thời gian thực khi dịch truyền sắp hết.

Thiết bị sử dụng vi điều khiển **ESP32 NodeMCU DevKit V1**, cảm biến ngắt hồng ngoại **ITR9606** (đếm giọt), cảm biến trọng lượng **Loadcell 5kg + HX711** và màn hình **OLED 0.66"** để hiển thị tại chỗ. Dữ liệu được đồng bộ lên **ThingsBoard Cloud** qua giao thức MQTT.

---

## 1. Chức năng hệ thống

* **Đo lưu lượng dịch truyền:**
  * Sử dụng ngắt cứng (External Interrupt) đếm xung từ cảm biến ITR9606 kẹp tại bầu nhỏ giọt, đảm bảo không bỏ sót xung giọt nào.
  * Tự động tính toán tốc độ nhỏ giọt tức thời **DPM** (Drops Per Minute).
  * Quy đổi sang lưu lượng thể tích **ml/h** (mili-lít/giờ) dựa trên hệ số giọt chuẩn của dây truyền ($20 \text{ giọt} = 1 \text{ ml}$).
* **Đo khối lượng dịch truyền:**
  * Loadcell 5kg kết hợp mạch ADC 24-bit HX711 liên tục giám sát trọng lượng chai dịch truyền.
  * Hỗ trợ tự động trừ bì (Tare) cốc/chai rỗng khi khởi động thiết bị.
* **Cảnh báo khẩn cấp:**
  * Tự động kích hoạt còi báo (Buzzer) kêu bíp ngắt quãng khi trọng lượng dịch truyền trong chai giảm xuống dưới ngưỡng tới hạn **50g**.
  * Chữ cảnh báo **ALERT** nhấp nháy liên tục trên màn hình OLED để nhân viên y tế dễ dàng nhận biết.
* **Giám sát từ xa (IoT Cloud):**
  * Kết nối Wi-Fi và gửi dữ liệu telemetry lên ThingsBoard Cloud qua giao thức MQTT.
  * Cho phép theo dõi đồ thị cân nặng, tốc độ truyền dịch và trạng thái cảnh báo trên Dashboard thời gian thực.

---

## 2. Sơ đồ kết nối phần cứng (Pinout)

Kết nối dây giữa các linh kiện và mạch ESP32 trên PCB:

| Linh kiện | Tên chân trên mạch | Chân kết nối ESP32 | Ghi chú |
| :--- | :--- | :--- | :--- |
| **Cảm biến ITR9606** | `DROP_COUNT` | **GPIO18** | Chân ngắt ngoài đếm giọt |
| **Mạch ADC HX711** | `HX711_DT` (Data) | **GPIO16 (RX2)** | Truyền dữ liệu cân nặng |
| **Mạch ADC HX711** | `HX711_SCK` (Clock) | **GPIO17 (TX2)** | Xung nhịp đồng bộ HX711 |
| **Màn hình OLED I2C** | `OLED_SDA` | **GPIO21** | Đường truyền dữ liệu SDA |
| **Màn hình OLED I2C** | `OLED_SCL` | **GPIO22** | Đường xung nhịp SCL |
| **Còi báo động (Buzzer)** | `BUZZER_CTRL` | **GPIO19** | Điều khiển còi qua transistor |

---

## 3. Thư viện sử dụng (Arduino IDE)

Để nạp code và chạy chương trình, cần cài đặt các thư viện sau trên Arduino IDE:
* **Adafruit SSD1306** & **Adafruit GFX** (Điều khiển màn hình OLED)
* **HX711** (Giao tiếp cảm biến lực - tác giả *bogde*)
* **PubSubClient** (Giao thức MQTT kết nối ThingsBoard)

---

## 4. Cấu hình nhanh thiết bị

Mở file `SmartInfusionMonitor/SmartInfusionMonitor.ino` và thay đổi các cấu hình mạng:

```cpp
// Thông tin Wi-Fi
const char* WIFI_SSID     = "TÊN_WIFI";
const char* WIFI_PASSWORD = "MẬT_KHẨU_WIFI";

// Thông tin ThingsBoard MQTT
const char* TB_SERVER   = "mqtt.eu.thingsboard.cloud";
const int   TB_PORT     = 1883;
const char* TB_TOKEN    = "ACCESS_TOKEN_THIẾT_BỊ"; // Token lấy từ ThingsBoard
```

---

## 5. Thiết kế phần cứng (Altium Designer)

Toàn bộ mạch nguyên lý và thiết kế mạch in được lưu trữ trong thư mục `PCB_Project/`:
* `Sheet1.SchDoc`: Mạch nguyên lý chi tiết (Khối điều khiển ESP32, khối nguồn hạ áp, khối cảm biến và còi báo động).
* `PCB1.PcbDoc`: Thiết kế mạch in 2 lớp đi dây thực tế.
* `Truyen_dich.PrjPcb`: Quản lý dự án Altium.
