// =========================================================================
// Симулятор протокола обмена данными для "CLST2 Cartridge System"
// Версия 1.2: Просторный UI с визуализацией шины
// Платформа: Processing
// =========================================================================

KPK kpk;
Cartridge cartridge;
Bus bus;

String statusText = "Готов. Нажмите 'Читать ID'.";
boolean isReading = false;

// --- Главная "магическая" функция, имитирующая обмен по шине ---
char transferByte(int addressToSend) {
  // Анимируем процесс
  bus.animateTransfer(addressToSend, cartridge.getDataAtAddress(addressToSend));
  return cartridge.getDataAtAddress(addressToSend);
}

void setup() {
  size(1200, 700); // Увеличиваем размер холста
  
  kpk = new KPK();
  cartridge = new Cartridge("TYPE=PRINTER;NAME=SIG-MA v1;DRVID=00000001;CARDID=A1B2C3D4;");
  bus = new Bus();
}

void draw() {
  background(40, 40, 55);
  
  // Обновляем анимацию шины
  bus.update();
  
  // Читаем один байт за раз для наглядности
  if (isReading && frameCount % 30 == 0) { // Замедлим чтение для анимации
    kpk.readNextByte();
  }
  
  kpk.display();
  cartridge.display();
  bus.display();
  drawUI();
}

void mousePressed() {
  if (mouseX > 50 && mouseX < 180 && mouseY > 620 && mouseY < 680) {
    if (!isReading) {
      kpk.startReadSequence();
      isReading = true;
    }
  }
}

void drawUI() {
  fill(isReading ? 100 : #22B14C);
  rect(50, 620, 130, 60, 5);
  fill(255);
  textSize(20);
  textAlign(CENTER, CENTER);
  text("Читать ID", 115, 650);
  
  textAlign(LEFT, TOP);
  textSize(16);
  fill(200);
  text("Статус: " + statusText, 200, 630);
}

// =========================================================================
// Класс Шины (Bus) - с анимацией
// =========================================================================
class Bus {
  int data = 0;
  color[] bitColors = new color[4];
  float animationProgress = 1.0;
  
  Bus() {
    for (int i = 0; i < 4; i++) bitColors[i] = color(80);
  }
  
  // Запускает анимацию передачи байта
  void animateTransfer(int address, char dataByte) {
    this.data = (int)dataByte; // Отображаем принимаемый байт
    animationProgress = 0.0;
  }
  
  void update() {
    if (animationProgress < 1.0) {
      animationProgress += 0.05; // Скорость анимации
    }
  }
  
  void display() {
    textAlign(CENTER, CENTER);
    textSize(14);
    
    float pulse = (0.5 + sin(animationProgress * PI) * 0.5); // 0 -> 1 -> 0
    
    // Шина данных D0-D3
    for (int i = 0; i < 4; i++) {
      boolean bitIsSet = ((data >> i) & 1) == 1;
      
      // Анимируем цвет
      color baseColor = color(80);
      color onColor = color(255, 255, 0);
      bitColors[i] = lerpColor(baseColor, onColor, bitIsSet ? pulse : 0);
      
      stroke(bitColors[i]);
      strokeWeight(bitIsSet ? 3 * pulse + 1 : 1);
      
      line(350, 300 + i*30, 850, 300 + i*30);
      fill(bitColors[i]);
      text("D" + i, 600, 285 + i*30);
    }
  }
}

// =========================================================================
// Класс Картриджа (Увеличенный)
// =========================================================================
class Cartridge {
  String passport;
  
  Cartridge(String p) {
    passport = p;
  }
  
  char getDataAtAddress(int address) {
    if (address < passport.length()) {
      return passport.charAt(address);
    }
    return 0;
  }
  
  void display() {
    fill(80, 0, 0);
    stroke(200, 50, 50);
    rect(700, 50, 450, 150, 10);
    
    fill(255);
    textSize(24);
    textAlign(CENTER, CENTER);
    text("КАРТРИДЖ (Устройство)", 925, 80);
    
    textSize(16);
    textAlign(LEFT, TOP);
    text("Хранимый паспорт:\n" + passport, 720, 120);
  }
}

// =========================================================================
// Класс КПК (Увеличенный)
// =========================================================================
class KPK {
  String receivedData = "";
  int readAddress = 0;
  
  void startReadSequence() {
    receivedData = "";
    readAddress = 0;
    statusText = "Начинаю чтение...";
  }
  
  void readNextByte() {
    char receivedChar = transferByte(readAddress);
    
    if (receivedChar == 0) {
      statusText = "Чтение завершено! Получено: " + receivedData;
      isReading = false;
      return;
    }
    
    receivedData += receivedChar;
    readAddress++;
    statusText = "Читаю байт #" + (readAddress) + "...";
  }
  
  void display() {
    fill(0, 80, 0);
    stroke(50, 200, 50);
    rect(50, 50, 450, 200, 10);
    
    fill(255);
    textSize(24);
    textAlign(CENTER, CENTER);
    text("КПК (Хост)", 275, 80);
    
    textSize(16);
    textAlign(LEFT, TOP);
    text("Полученные данные:\n" + receivedData, 70, 120);
  }
}
