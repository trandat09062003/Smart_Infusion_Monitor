#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

// Kích thước màn hình OLED 0.66 inch
#define SCREEN_WIDTH  64
#define SCREEN_HEIGHT 48
#define OLED_RESET    -1

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

// Mảng chứa giá trị sóng điện tim ECG giả lập để cuộn màn hình
int ecgPoints[40];
int ecgIndex = 0;

// Trạng thái hoạt động
int progress = 0;
int dropY = 32;
int bagFill = 15;

void setup() {
  Serial.begin(115200);
  
  // Khởi tạo I2C tùy biến theo Schematic của bạn: SDA=21, SCL=22
  Wire.begin(21, 22);

  if(!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    Serial.println(F("OLED allocation failed"));
    for(;;);
  }

  display.clearDisplay();
  display.display();
  
  // Khởi tạo mảng sóng ECG phẳng ban đầu
  for (int i = 0; i < 40; i++) {
    ecgPoints[i] = 38; // Đường nền phẳng ở tọa độ Y = 38
  }
}

// Hàm vẽ biểu tượng túi dịch truyền có hoạt ảnh nhỏ giọt
void drawInfusionBag(int x, int y, int fillLevel, int dropPos) {
  // Vẽ móc treo túi dịch
  display.drawCircle(x + 5, y, 2, SSD1306_WHITE);
  
  // Vẽ thân túi dịch truyền
  display.drawRoundRect(x, y + 2, 11, 20, 2, SSD1306_WHITE);
  
  // Vẽ lượng dịch còn lại bên trong túi (nạp động)
  if (fillLevel > 0) {
    display.fillRect(x + 2, y + 2 + (18 - fillLevel), 7, fillLevel, SSD1306_WHITE);
  }
  
  // Vẽ đầu vòi nhỏ giọt
  display.drawRect(x + 4, y + 22, 3, 3, SSD1306_WHITE);
  
  // Hoạt ảnh giọt nước rơi
  display.fillCircle(x + 5, y + 25 + dropPos, 1, SSD1306_WHITE);
}

// Hàm tạo điểm sóng tim ECG ngẫu nhiên (nhịp tim QRS)
int getECGPoint() {
  static int step = 0;
  step++;
  if (step >= 20) step = 0; // Chu kỳ nhịp tim

  if (step == 5) return 28;  // Sóng P nhẹ
  if (step == 8) return 38;  // Trở về nền
  if (step == 9) return 44;  // Sóng Q âm sâu
  if (step == 10) return 16; // Sóng R dương cao vút (QRS peak)
  if (step == 11) return 46; // Sóng S âm sâu
  if (step == 12) return 38; // Trở về nền
  if (step == 14) return 32; // Sóng T
  
  return 38; // Đường đẳng điện phẳng (Baseline)
}

void loop() {
  // ==========================================
  // PHASE 1: CHẠY ANIMATION KHỞI ĐỘNG (BOOT SCREEN)
  // ==========================================
  for (progress = 0; progress <= 100; progress += 4) {
    display.clearDisplay();
    
    // Viết chữ "HUST" hoặc tiêu đề đồ án
    display.setTextSize(1);
    display.setTextColor(SSD1306_WHITE);
    display.setCursor(8, 2);
    display.print("HUST IoT");
    
    display.setCursor(4, 14);
    display.print("LOADING");
    
    // Vẽ thanh tiến trình chạy (Loading Progress Bar)
    display.drawRect(4, 28, 56, 8, SSD1306_WHITE);
    display.fillRect(6, 30, (52 * progress) / 100, 4, SSD1306_WHITE);
    
    // Hiển thị phần trăm %
    display.setCursor(20, 39);
    display.print(progress);
    display.print("%");
    
    display.display();
    delay(50);
  }
  delay(800);

  // ==========================================
  // PHASE 2: GIAO DIỆN Y TẾ THỜI GIAN THỰC GIẢ LẬP (MEDICAL SIMULATION)
  // ==========================================
  unsigned long demoStartTime = millis();
  while (millis() - demoStartTime < 15000) { // Chạy demo giao diện y tế trong 15 giây
    display.clearDisplay();
    
    // 1. Vẽ khung viền bảo vệ ngoài cùng
    display.drawRect(0, 0, 64, 48, SSD1306_WHITE);
    
    // 2. Cập nhật hoạt ảnh túi dịch truyền (Bag filling/emptying)
    static unsigned long lastBagAnim = 0;
    if (millis() - lastBagAnim > 1000) {
      lastBagAnim = millis();
      bagFill--; // Chai dịch truyền vơi dần
      if (bagFill < 2) bagFill = 16; // Tự reset đầy lại để test tiếp
    }
    
    // Cập nhật hoạt ảnh giọt nước rơi
    dropY += 2;
    if (dropY > 12) dropY = 0;
    
    // Vẽ túi dịch truyền ở góc trái màn hình
    drawInfusionBag(4, 4, bagFill, dropY);
    
    // 3. Hiển thị thông số lưu lượng và cảnh báo
    display.setTextSize(1);
    display.setTextColor(SSD1306_WHITE);
    
    display.setCursor(20, 4);
    display.print("FLOW");
    
    display.setCursor(20, 13);
    display.print("45 dpm"); // Giả lập 45 giọt/phút
    
    // Đèn nháy trạng thái hoạt động (nhấp nháy chấm tròn)
    if ((millis() / 500) % 2 == 0) {
      display.fillCircle(57, 7, 2, SSD1306_WHITE);
    }
    
    // 4. Tạo hiệu ứng cuộn sóng tim điện tâm đồ (ECG Waveform Scroll)
    // Dịch các điểm ECG sang trái 1 pixel
    for (int i = 0; i < 39; i++) {
      ecgPoints[i] = ecgPoints[i + 1];
    }
    // Lấy điểm sóng tim mới nhất cho vào cuối mảng
    ecgPoints[39] = getECGPoint();
    
    // Vẽ các đường nối tiếp sóng tim ECG cuộn màn hình
    for (int i = 20; i < 39; i++) {
      // Bắt đầu vẽ từ cột X = 20 (chừa góc trái cho hình túi dịch truyền) đến X = 60
      int xStart = 20 + (i - 20);
      display.drawLine(xStart, ecgPoints[i], xStart + 1, ecgPoints[i + 1], SSD1306_WHITE);
    }
    
    display.display();
    delay(40); // Tốc độ cuộn sóng tim mượt mà ~25 khung hình/giây
  }
}
