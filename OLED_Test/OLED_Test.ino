#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

// 1. Cấu hình kích thước màn hình OLED 0.66 inch
#define SCREEN_WIDTH  64
#define SCREEN_HEIGHT 48
#define OLED_RESET    -1  // Không dùng chân reset riêng

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

void setup() {
  Serial.begin(115200);
  Serial.println(F("=== OLED 0.66'' 64x48 Test ==="));

  // 2. Khởi tạo giao tiếp I2C
  // TRƯỜNG HỢP A: Nếu test trên mạch in (PCB) của bạn (đã đảo chân: SDA=23, SCL=21)
  Wire.begin(23, 21);
  
  // TRƯỜNG HỢP B: Nếu test trên kit ESP32 thông thường ngoài breadboard (chân mặc định SDA=21, SCL=22)
  // Bạn hãy uncomment dòng dưới đây và comment dòng Wire.begin(21,23) ở trên lại nhé:
  // Wire.begin(); 

  // 3. Khởi tạo màn hình OLED (Địa chỉ I2C thông dụng của SSD1306 là 0x3C)
  if(!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    Serial.println(F("[ERROR] Không tìm thấy màn hình OLED!"));
    while(1); // Dừng chương trình nếu lỗi
  }

  Serial.println(F("[OK] Khởi tạo OLED thành công!"));
  
  // Xóa bộ đệm màn hình ban đầu
  display.clearDisplay();
  display.display();
  delay(500);

  // --- BÀI TEST 1: Hiển thị chữ tiếng Anh ---
  display.setTextSize(1);             // Cỡ chữ nhỏ nhất (chứa được khoảng 8 ký tự/dòng, tối đa 5 dòng)
  display.setTextColor(SSD1306_WHITE); // Màu chữ trắng (nền đen)
  
  display.setCursor(0, 0);
  display.println("HELLO!");
  
  display.setCursor(0, 10);
  display.println("ESP32 OK");
  
  display.setCursor(0, 20);
  display.println("OLED0.66");
  
  display.setCursor(0, 30);
  display.println("64x48 px");
  
  display.setCursor(0, 40);
  display.println("READY!");
  
  display.display(); // Vẽ ra màn hình các thay đổi ở trên
  delay(3000);       // Giữ màn hình này trong 3 giây
}

void loop() {
  // --- BÀI TEST 2: Hiệu ứng đảo màu toàn bộ màn hình ---
  // Invert Display: Bật chế độ hiển thị âm bản (chữ đen nền trắng)
  display.invertDisplay(true);
  delay(1000);
  
  // Normal Display: Trở về bình thường (chữ trắng nền đen)
  display.invertDisplay(false);
  delay(1000);
}
