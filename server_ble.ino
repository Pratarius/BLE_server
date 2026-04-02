/**
 * @file BLE_Server.ino
 * @brief Bluetooth Low Energy (BLE) Server implementation for ESP32.
 * @details This firmware configures an ESP32 as a BLE Peripheral (Server). 
 * It utilizes the NimBLE stack for lower memory consumption. The device 
 * broadcasts its presence, accepts client connections, and uses hardware 
 * interrupts (callbacks) to handle asynchronous read/write data events.
 */

#include <NimBLEDevice.h>

// Unique Identifiers (UUIDs) for the BLE Architecture
// SERVICE_UUID represents the "Folder" grouping our data.
// CHARACTERISTIC_UUID represents the actual "File" holding the data payload.
#define SERVICE_UUID        "4fafc201-1fb5-459e-8fcc-c5c9c331914b"
#define CHARACTERISTIC_UUID "beb5483e-36e1-4688-b7f5-ea07361b26a8"

// The default string loaded into memory on boot and upon client disconnect
const char* ORIGINAL_VALUE = "Hello from NimBLE";  

// Global pointer to our data characteristic so it can be accessed by both setup() and callbacks
extern NimBLECharacteristic *pCharacteristic;

/* -------------------------------------------------------------------------- */
/* CONNECTION EVENT CALLBACKS                          */
/* -------------------------------------------------------------------------- */

/**
 * @class ServerCallbacks
 * @brief Handles background events for BLE radio connections and disconnections.
 */
class ServerCallbacks : public NimBLEServerCallbacks {
    
    // Triggered the exact millisecond a BLE Client (Central) connects to this ESP32
    void onConnect(NimBLEServer *pServer, NimBLEConnInfo &connInfo) {
        Serial.println("Client connected!");
        Serial.print("Client MAC address: ");
        Serial.println(connInfo.getAddress().toString().c_str());
    }

    // Triggered the exact millisecond a BLE Client drops the connection or goes out of range
    void onDisconnect(NimBLEServer *pServer, NimBLEConnInfo &connInfo, int reason) {
        Serial.print("Client disconnected, reason code: ");
        Serial.println(reason);
        
        // Housekeeping: Reset the data buffer back to the default state
        pCharacteristic->setValue(ORIGINAL_VALUE);  
        Serial.println("Value reset to original");
        
        // CRITICAL HARDWARE BEHAVIOR: 
        // A BLE Server stops broadcasting its existence once a client connects.
        // We must manually command the radio to resume advertising so new clients can find us.
        NimBLEDevice::startAdvertising();            
        Serial.println("Advertising restarted. Waiting for new clients...");
    }
};

/* -------------------------------------------------------------------------- */
/* DATA EVENT CALLBACKS                             */
/* -------------------------------------------------------------------------- */

/**
 * @class CharCallbacks
 * @brief Handles incoming and outgoing data requests on our specific Characteristic.
 */
class CharCallbacks : public NimBLECharacteristicCallbacks {
    
    // Triggered when a connected Client pushes new data (Write Request) to the ESP32
    void onWrite(NimBLECharacteristic *pCharacteristic, NimBLEConnInfo &connInfo) {
        Serial.println(">>> onWrite FIRED: Incoming Data Detected <<<");
        
        // Fetch the payload from the hardware memory buffer
        std::string val = pCharacteristic->getValue();
        Serial.print("Received Value: ");
        Serial.println(val.c_str());
        
        // NOTE: Hardware control logic (e.g., digitalWrite for an LED) goes here.
    }

    // Triggered when a connected Client explicitly requests to read the current memory buffer
    void onRead(NimBLECharacteristic *pCharacteristic, NimBLEConnInfo &connInfo) {
        Serial.println(">>> onRead FIRED: Client requested data dump <<<");
    }
};

/* -------------------------------------------------------------------------- */
/* GLOBAL INSTANTIATIONS                            */
/* -------------------------------------------------------------------------- */

ServerCallbacks serverCallbacks;
CharCallbacks charCallbacks;
NimBLECharacteristic *pCharacteristic = nullptr;

/* -------------------------------------------------------------------------- */
/* MAIN SETUP                                  */
/* -------------------------------------------------------------------------- */

void setup() {
    // 1. Initialize UART for serial debugging
    Serial.begin(115200);
    delay(1000); // Allow hardware to stabilize
    Serial.println("=== BOOTING BLE SERVER ===");

    // 2. Turn on the BLE radio and set the public broadcast name
    NimBLEDevice::init("NimBLE ESP32");

    // 3. Build the Server and attach connection interrupt handlers
    NimBLEServer *pServer = NimBLEDevice::createServer();
    pServer->setCallbacks(&serverCallbacks);
    Serial.println("Server architecture built and connection callbacks attached.");

    // 4. Build the Service (The Folder)
    NimBLEService *pService = pServer->createService(SERVICE_UUID);

    // 5. Build the Characteristic (The File) with explicit Read/Write permissions
    pCharacteristic = pService->createCharacteristic(
        CHARACTERISTIC_UUID,
        NIMBLE_PROPERTY::READ |
        NIMBLE_PROPERTY::WRITE
    );

    // 6. Attach data interrupt handlers and inject initial string into memory
    pCharacteristic->setCallbacks(&charCallbacks);
    pCharacteristic->setValue(ORIGINAL_VALUE);
    Serial.println("Data characteristic established and callbacks attached.");

    // 7. Ignite the sequence: Start the service and begin broadcasting radio pulses
    pService->start();
    NimBLEDevice::getAdvertising()->start();
    Serial.println("=== SYSTEM READY & ADVERTISING ===");
}

/* -------------------------------------------------------------------------- */
/* MAIN LOOP                                   */
/* -------------------------------------------------------------------------- */

void loop() {
    // Because BLE data handling is entirely managed by background hardware interrupts 
    // (the Callbacks), the main loop does not need to poll for data. 
    // This loop simply acts as a 3-second heartbeat to visually verify the current state.
    
    if (pCharacteristic != nullptr) {
        std::string val = pCharacteristic->getValue();
        Serial.print("Polled value from buffer: ");
        Serial.println(val.c_str());
    }
    
    delay(3000);
}