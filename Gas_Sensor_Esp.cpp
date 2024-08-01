#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>
#include <BLE2902.h>
#include <string.h>

#define SERVICE_UUID "4fafc201-1fb5-459e-8fcc-c5c9c331914b"
#define CHARACTERISTIC_UUID "8904b455-2e64-497c-82e9-b441cd3b7368"
#define GAS_SENSOR_PIN 7 // Pin connected to the gas sensor
#define THRESHOLD 1670 // Threshold value for gas detection

bool deviceConnected = false;
bool gasLeakDetected = false;

// Classe que lida com eventos de conexão e desconexão
class MyServerCallbacks: public BLEServerCallbacks {
  void onConnect(BLEServer *pServer) {
    Serial.println("  Usuário novo conectado  ");
    deviceConnected = true;
  };

  void onDisconnect(BLEServer *pServer) {
    Serial.println("  Usuário desconectado  ");
    deviceConnected = false;
  }
};

// Classe para lidar com callbacks de leitura e escrita
class MyCallback : public BLECharacteristicCallbacks {
  void onRead(BLECharacteristic *pCharacteristic) {
    Serial.println("Pediu algo");
  }

  void onWrite(BLECharacteristic *pCharacteristic) {
    Serial.println("Pediu para escrever algo");
  }
};

// Classe para a característica de valor do sensor
class MySensorCharacteristic: public BLECharacteristic {
  public: 
    MySensorCharacteristic(): BLECharacteristic(BLEUUID(CHARACTERISTIC_UUID), BLECharacteristic::PROPERTY_NOTIFY | BLECharacteristic::PROPERTY_READ) {
      setLeakStatus(false);
    }

    // Função que atualiza o status de vazamento no BLE
    void setLeakStatus(bool status) {
      this->leakStatus = status;
      const char* statusStr = status ? "Vazamento detectado" : "Sem vazamento";
      setValue(statusStr);
    }

    // Função que pega o status de vazamento do BLE
    std::string getLeakStatus() {
      return this->leakStatus ?  "Vazamento detectado" : "Sem vazamento";;
    }

  private:
    bool leakStatus;
};

// Declarando Variaveis Globais
MySensorCharacteristic *pCharacteristic;

void setup() {
  Serial.begin(115200);

  // Inicializa um dispositivo BLE
  BLEDevice::init("ESP32_GasSensor");
  Serial.println("Device BLE iniciado...");

  // Cria um servidor BLE
  BLEServer *pServer = BLEDevice::createServer();
  pServer->setCallbacks(new MyServerCallbacks());

  // Cria um serviço BLE
  BLEService *pService = pServer->createService(SERVICE_UUID);
  Serial.println("\tServiço BLE iniciado...");

  // Cria a característica do sensor e adiciona ao serviço
  pCharacteristic = new MySensorCharacteristic();
  pService->addCharacteristic(pCharacteristic);
  Serial.println("\tCaracterística BLE iniciada...");

  pCharacteristic->addDescriptor(new BLE2902());
  pCharacteristic->setCallbacks(new MyCallback());

  pService->start();

  // Começa a anunciar o servidor BLE
  BLEAdvertising *pAdvertising = BLEDevice::getAdvertising();
  pAdvertising->addServiceUUID(SERVICE_UUID);
  pAdvertising->setScanResponse(true);
  pAdvertising->setMinPreferred(0x06);  // Funções que ajudam com problemas de conexão com iPhone
  pAdvertising->setMinPreferred(0x12);
  BLEDevice::startAdvertising();
  
  Serial.println("Esperando cliente conectar...");

  // Configura o pino do sensor de gás como entrada
  //pinMode(GAS_SENSOR_PIN, INPUT);
}

void loop() {
  BLEDevice::setPower(ESP_PWR_LVL_N0);
  if (deviceConnected) {
    // Lê o valor do sensor de gás
    uint32_t sensorValue = analogRead(GAS_SENSOR_PIN);
    delay(300);
    int aux1 = analogRead(GAS_SENSOR_PIN);
    Serial.print("Gas Sensor Value: ");
    Serial.println(aux1);
    sensorValue = aux1;

    // Verifica se há vazamento
    bool currentLeakStatus = (sensorValue > THRESHOLD);
    
    if (currentLeakStatus != gasLeakDetected) {
      gasLeakDetected = currentLeakStatus;
      
      // Atualiza e notifica o status de vazamento quando um cliente está conectado
      pCharacteristic->setLeakStatus(gasLeakDetected);
      pCharacteristic->notify();
      Serial.print("Notifiquei meu cliente com o status: ");
      Serial.println(gasLeakDetected ? "Vazamento detectado" : "Sem vazamento");
    }

    delay(3000); // Espera um pouco antes de enviar o próximo valor
  }
}