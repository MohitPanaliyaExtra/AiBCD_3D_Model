#include <WiFi.h>
#include <WebServer.h>
#include <Adafruit_NeoPixel.h>
#include <LittleFS.h>


// LED Configuration
#define LEFT_LED_PIN 23       // GPIO pin for left LED strip
#define RIGHT_LED_PIN 22      // GPIO pin for right LED strip
#define NUM_LEDS_PER_STRIP 47 // LEDs per strip
#define TOTAL_LEDS 94         // Total LEDs (47 left + 47 right)

// WiFi AP Configuration
const char* ssid = "DigitalSweep-BCD";
const char* password = "Digital@1052";

// Objects
Adafruit_NeoPixel leftStrip(NUM_LEDS_PER_STRIP, LEFT_LED_PIN, NEO_GRB + NEO_KHZ800);
Adafruit_NeoPixel rightStrip(NUM_LEDS_PER_STRIP, RIGHT_LED_PIN, NEO_GRB + NEO_KHZ800);
WebServer server(80);

// LED State
bool ledStates[TOTAL_LEDS];
uint8_t ledBrightness[TOTAL_LEDS];  // Individual brightness for each LED
uint8_t leftMasterBrightness = 50;   // Master brightness for all left LEDs
uint8_t rightMasterBrightness = 50;  // Master brightness for all right LEDs

void setup() {
  Serial.begin(115200);
  
  // Initialize LED strips
  leftStrip.begin();
  leftStrip.clear();
  leftStrip.show();
  
  rightStrip.begin();
  rightStrip.clear();
  rightStrip.show();
  
  // Initialize LED states and brightness
  for(int i = 0; i < TOTAL_LEDS; i++) {
    ledStates[i] = false;
    ledBrightness[i] = 50;  // Default individual brightness
  }
  
  // Setup WiFi AP
  WiFi.softAP(ssid, password);
  IPAddress IP = WiFi.softAPIP();
  Serial.print("AP IP address: ");
  Serial.println(IP);
  
  // Setup web server routes
  server.on("/", handleRoot);
  server.on("/toggleAllOn", handleToggleAllOn);
  server.on("/resetSide", handleResetSide);
  server.on("/setMasterBrightness", handleSetMasterBrightness);
  server.on("/toggleLED", handleToggleLED);
  server.on("/setLEDBrightness", handleSetLEDBrightness);
  server.on("/getState", handleGetState);

  server.begin();
  if(!LittleFS.begin(true)) {
  Serial.println("LittleFS Mount Failed");
}
server.on("/logo.png", []() {
  File file = LittleFS.open("/logo.png", "r");
  if (!file) {
    server.send(404, "text/plain", "File not found");
    return;
  }
  server.streamFile(file, "image/png");
  file.close();
});

  Serial.println("HTTP server started");
}

void loop() {
  server.handleClient();
  updateLEDs();
}

void updateLEDs() {
  // Update left strip (LEDs 0-46)
  for(int i = 0; i < NUM_LEDS_PER_STRIP; i++) {
    if(ledStates[i]) {
      // Individual LED brightness is independent, master brightness doesn't affect it
      leftStrip.setPixelColor(i, leftStrip.Color(ledBrightness[i], ledBrightness[i], ledBrightness[i]));
    } else {
      leftStrip.setPixelColor(i, leftStrip.Color(0, 0, 0));
    }
  }
  leftStrip.show();
  
  // Update right strip (LEDs 47-93)
  for(int i = 0; i < NUM_LEDS_PER_STRIP; i++) {
    int ledIndex = i + NUM_LEDS_PER_STRIP;
    if(ledStates[ledIndex]) {
      // Individual LED brightness is independent, master brightness doesn't affect it
      rightStrip.setPixelColor(i, rightStrip.Color(ledBrightness[ledIndex], ledBrightness[ledIndex], ledBrightness[ledIndex]));
    } else {
      rightStrip.setPixelColor(i, rightStrip.Color(0, 0, 0));
    }
  }
  rightStrip.show();
}

void handleRoot() {
  String html = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
  <meta name="viewport" content="width=device-width, initial-scale=1">
  <title>LED Control</title>
  <style>
   * { margin: 0; padding: 0; box-sizing: border-box; }
    body {
      font-family: Arial, sans-serif;
      background: white;
      padding: 10px;
      min-height: 100vh;
    }
    .container { max-width: 1600px; margin: 0 auto; }
      .header {
      background: White;
      padding: 10px;
      /* border-bottom: 1px solid black; */
      display:flex;
      padding-bottom: 20px;
      padding-top: 20px;
      justify-content: center;
    }
.header .logo{
  position: absolute;
  top:5px;
  left:130px;
}

    h1 {
      /* display:none; */
      color: Black;
      font-size: 28px;
      text-align: center;
      align-self:center;
      /* margin-left: auto; */
      /* margin-right: auto; */
    }
    
    .dual-section {
      display: flex;
      gap: 40px;
      justify-content: center;
      flex-wrap: wrap;
      margin-bottom: 10px;
    }
    
    .side-panel {
      background: rgba(255,255,255,0.1);
      padding: 10px;
      flex: 1;
      min-width: 350px;
      max-width: 500px;
    }
    
    .side-title {
      color: Black;
      font-size: 22px;
      font-weight: bold;
      margin-bottom: 10px;
      text-align: center;
    }
    
    .control-group {
      margin-bottom: 20px;
    }
    
    .control-label {
      color: Black;
      font-size: 14px;
      margin-bottom: 8px;
      display: flex;
      justify-content: space-between;
      align-items: center;
    }
    
    .control-value {
      color: #4CAF50;
      font-weight: bold;
      font-size: 14px;
    }
    
    input[type="range"] {
      width: 100%;
      height: 8px;
      border-radius: 5px;
      background: #444;
      outline: none;
      cursor: pointer;
    }
    
    input[type="range"]::-webkit-slider-thumb {
      appearance: none;
      width: 20px;
      height: 20px;
      border-radius: 50%;
      background: #4CAF50;
      cursor: pointer;
    }
    
    .button-group {
      display: flex;
      gap: 10px;
      margin-top: 15px;
    }
    
    .btn {
      flex: 1;
      padding: 12px;
      border: none;
      border-radius: 8px;
      font-size: 14px;
      font-weight: bold;
      cursor: pointer;
      transition: all 0.3s;
    }
    
    .btn-primary {
      background: #4CAF50;
      color: white;
    }
    
    .btn-primary:hover {
      background: white;
      border: 1px solid #45a049;
      color: #45a049;
    }
    
    .btn-secondary {
      background: #2196F3;
      color: white;
    }
    
    .btn-secondary:hover {
      background: white;
      border: 1px solid #0b7dda;
      color: #0b7dda;
    }
    
    .led-display {
      padding: 40px;
      border-radius: 15px;
      display: flex;
      justify-content: center;
      align-items: center;
    }
    
    .led-circle-container {
      position: relative;
      width: 400px;
      height: 400px;
    }
    
    .led-dot {
      width: 35px;
      height: 35px;
      border-radius: 50%;
      background: white;
      border: 2px solid #666;
      position: absolute;
      cursor: pointer;
      display: flex;
      align-items: center;
      justify-content: center;
      font-size: 11px;
      color: black;
      transition: all 0.2s;
      user-select: none;
    }
    
    .led-dot:hover {
      transform: scale(1.15);
      border-color: black;
    }
    
    .led-dot.on {
      background: #ffff00;
      border-color: #ffaa00;
      box-shadow: 0 0 15px #ffff00;
    }
    
    // .led-dot.selected {
    //   border: 3px solid #FF1744;
    //   box-shadow: 0 0 20px #FF1744;
    // }
    
    .individual-control {
      background: rgba(76, 175, 80, 0.1);
      padding: 15px;
      border-radius: 10px;
      margin-bottom: 20px;
      border: 2px solid #4CAF50;
    }
    
    .individual-control.disabled {
      opacity: 0.5;
      pointer-events: none;
    }
    
    .individual-title {
      color: #4CAF50;
      font-size: 16px;
      font-weight: bold;
      margin-bottom: 10px;
      text-align: center;
    }
    
    .left, .right {
      display: flex;
      flex-direction: column;
      gap: 10px;
    }
  </style>
</head>
<body>
  <div class="container">
    <div class="header">
    <img class="logo" src="/logo.png" width="200"/>
      <h1>Control Panel</h1>
    </div>
    
    <div class="dual-section">
      <div class="left">
        <div class="led-display">
          <div class="led-circle-container" id="leftLEDContainer"></div>
        </div>
        <div class="side-panel">
          <div class="side-title">Left Side</div>
          
          <div class="individual-control disabled" id="leftIndividualControl">
            <div class="individual-title" id="leftIndividualTitle">Select an LED</div>
            <div class="control-group">
              <div class="control-label">
                <span>LED Brightness</span>
                <span class="control-value" id="leftLEDBrightnessValue">50</span>
              </div>
              <input type="range" min="0" max="255" value="50" id="leftLEDBrightnessSlider"
                     oninput="setLEDBrightness('left')">
            </div>
          </div>
          
          <div class="control-group">
            <div class="control-label">
              <span>Master Brightness</span>
              <span class="control-value" id="leftMasterBrightnessValue">50</span>
            </div>
            <input type="range" min="0" max="255" value="50" id="leftMasterBrightnessSlider"
                   oninput="setMasterBrightness('left', this.value)">
          </div>
          
          <div class="button-group">
            <button class="btn btn-primary" onclick="toggleAllOn('left')">All ON</button>
            <button class="btn btn-secondary" onclick="resetSide('left')">Reset</button>
          </div>
        </div>
      </div>
      
      <div class="right">
        <div class="led-display">
          <div class="led-circle-container" id="rightLEDContainer"></div>
        </div>
        <div class="side-panel">
          <div class="side-title">Right Side</div>
          
          <div class="individual-control disabled" id="rightIndividualControl">
            <div class="individual-title" id="rightIndividualTitle">Select an LED</div>
            <div class="control-group">
              <div class="control-label">
                <span>LED Brightness</span>
                <span class="control-value" id="rightLEDBrightnessValue">50</span>
              </div>
              <input type="range" min="0" max="255" value="50" id="rightLEDBrightnessSlider"
                     oninput="setLEDBrightness('right')">
            </div>
          </div>
          
          <div class="control-group">
            <div class="control-label">
              <span>Master Brightness</span>
              <span class="control-value" id="rightMasterBrightnessValue">50</span>
            </div>
            <input type="range" min="0" max="255" value="50" id="rightMasterBrightnessSlider"
                   oninput="setMasterBrightness('right', this.value)">
          </div>
          
          <div class="button-group">
            <button class="btn btn-primary" onclick="toggleAllOn('right')">All ON</button>
            <button class="btn btn-secondary" onclick="resetSide('right')">Reset</button>
          </div>
        </div>
      </div>
    </div>
  </div>

  <script>
    const NUM_LEDS = 47;
    
    // Separate states for left and right
    let leftLEDStates = new Array(NUM_LEDS).fill(false);
    let leftLEDBrightness = new Array(NUM_LEDS).fill(50);
    let rightLEDStates = new Array(NUM_LEDS).fill(false);
    let rightLEDBrightness = new Array(NUM_LEDS).fill(50);
    
    let leftMasterBrightness = 50;
    let rightMasterBrightness = 50;
    let selectedLeftLED = -1;
    let selectedRightLED = -1;
    
    function createLEDCircle(containerId, side) {
      const container = document.getElementById(containerId);
      const rings = [
        { radius: 188, count: 16 },
        { radius: 140, count: 16 },
        { radius: 89, count: 8 },
        { radius: 45, count: 6 },
        { radius: 0, count: 1 }
      ];
      let ANGLE_SHIFT=0.20;
      let ledIndex = 0;
      rings.forEach(ring => {
        for(let i = 0; i < ring.count && ledIndex < NUM_LEDS; i++) {
          let angle = (i / ring.count) * 2 * Math.PI - Math.PI / 2;
          if (ledIndex >= 16 && ledIndex <= 31) {
            angle += ANGLE_SHIFT;
          }
          const x = ring.radius * Math.cos(angle);
          const y = ring.radius * Math.sin(angle);
          
          const led = document.createElement('div');
          led.className = 'led-dot';
          led.id = side + '_led_' + ledIndex;
          led.textContent = ledIndex + 1;
          led.style.left = (x + 180) + 'px';
          led.style.top = (y + 180) + 'px';
          
          // Create a closure to capture the correct ledIndex
          (function(capturedIndex) {
            led.onclick = () => handleLEDClick(side, capturedIndex);
          })(ledIndex);
          
          container.appendChild(led);
          ledIndex++;
        }
      });
      
      console.log('Created', ledIndex, 'LEDs for', side, 'side');
    }
    
    function handleLEDClick(side, localIndex) {
      console.log('LED clicked - Side:', side, 'Local Index:', localIndex);
      
      // Validate index
      if(localIndex < 0 || localIndex >= NUM_LEDS) {
        console.error('Invalid LED index:', localIndex);
        return;
      }
      
      if(side === 'left') {
        // Deselect previous left LED
        if(selectedLeftLED >= 0 && selectedLeftLED !== localIndex) {
          const prevLED = document.getElementById('left_led_' + selectedLeftLED);
          if(prevLED) prevLED.classList.remove('selected');
        }
        
        // If clicking the same LED, toggle it and deselect
        if(selectedLeftLED === localIndex) {
          toggleLED(side, localIndex);
          selectedLeftLED = -1;
          document.getElementById('leftIndividualControl').classList.add('disabled');
          document.getElementById('leftIndividualTitle').textContent = 'Select an LED';
        } else {
          // Select new LED
          selectedLeftLED = localIndex;
          const currentLED = document.getElementById('left_led_' + localIndex);
          if(currentLED) {
            currentLED.classList.add('selected');
            document.getElementById('leftIndividualControl').classList.remove('disabled');
            document.getElementById('leftIndividualTitle').textContent = 'LED ' + (localIndex + 1);
            document.getElementById('leftLEDBrightnessSlider').value = leftLEDBrightness[localIndex];
            document.getElementById('leftLEDBrightnessValue').textContent = leftLEDBrightness[localIndex];
          } else {
            console.error('LED element not found: left_led_' + localIndex);
          }
        }
      } else {
        // Deselect previous right LED
        if(selectedRightLED >= 0 && selectedRightLED !== localIndex) {
          const prevLED = document.getElementById('right_led_' + selectedRightLED);
          if(prevLED) prevLED.classList.remove('selected');
        }
        
        // If clicking the same LED, toggle it and deselect
        if(selectedRightLED === localIndex) {
          toggleLED(side, localIndex);
          selectedRightLED = -1;
          document.getElementById('rightIndividualControl').classList.add('disabled');
          document.getElementById('rightIndividualTitle').textContent = 'Select an LED';
        } else {
          // Select new LED
          selectedRightLED = localIndex;
          const currentLED = document.getElementById('right_led_' + localIndex);
          if(currentLED) {
            currentLED.classList.add('selected');
            document.getElementById('rightIndividualControl').classList.remove('disabled');
            document.getElementById('rightIndividualTitle').textContent = 'LED ' + (localIndex + 1);
            document.getElementById('rightLEDBrightnessSlider').value = rightLEDBrightness[localIndex];
            document.getElementById('rightLEDBrightnessValue').textContent = rightLEDBrightness[localIndex];
          } else {
            console.error('LED element not found: right_led_' + localIndex);
          }
        }
      }
    }
    
    function toggleLED(side, localIndex) {
      // Convert local index to global index for API
      const globalIndex = side === 'left' ? localIndex : localIndex + NUM_LEDS;
      
      fetch(`/toggleLED?led=${globalIndex}`)
        .then(r => r.json())
        .then(data => {
          if(side === 'left') {
            leftLEDStates[localIndex] = data.state;
          } else {
            rightLEDStates[localIndex] = data.state;
          }
          updateLEDDisplay(side, localIndex);
        })
        .catch(err => console.error('Toggle error:', err));
    }
    
    function updateLEDDisplay(side, localIndex) {
      const led = document.getElementById(side + '_led_' + localIndex);
      if(led) {
        const isOn = side === 'left' ? leftLEDStates[localIndex] : rightLEDStates[localIndex];
        if(isOn) {
          led.classList.add('on');
        } else {
          led.classList.remove('on');
        }
      }
    }
    
    function setLEDBrightness(side) {
      const localIndex = side === 'left' ? selectedLeftLED : selectedRightLED;
      if(localIndex < 0) return;
      
      const slider = document.getElementById(side + 'LEDBrightnessSlider');
      const valueDisplay = document.getElementById(side + 'LEDBrightnessValue');
      
      const value = parseInt(slider.value);
      
      if(side === 'left') {
        leftLEDBrightness[localIndex] = value;
      } else {
        rightLEDBrightness[localIndex] = value;
      }
      
      valueDisplay.textContent = value;
      
      // Convert local index to global index for API
      const globalIndex = side === 'left' ? localIndex : localIndex + NUM_LEDS;
      fetch(`/setLEDBrightness?led=${globalIndex}&val=${value}`)
        .catch(err => console.error('Brightness error:', err));
    }
    
    function setMasterBrightness(side, value) {
      value = parseInt(value);
      
      if(side === 'left') {
        leftMasterBrightness = value;
        document.getElementById('leftMasterBrightnessValue').textContent = value;
        // Update all left LED brightness to match master
        for(let i = 0; i < NUM_LEDS; i++) {
          leftLEDBrightness[i] = value;
        }
        // Update selected LED slider if visible
        if(selectedLeftLED >= 0) {
          document.getElementById('leftLEDBrightnessSlider').value = value;
          document.getElementById('leftLEDBrightnessValue').textContent = value;
        }
      } else {
        rightMasterBrightness = value;
        document.getElementById('rightMasterBrightnessValue').textContent = value;
        // Update all right LED brightness to match master
        for(let i = 0; i < NUM_LEDS; i++) {
          rightLEDBrightness[i] = value;
        }
        // Update selected LED slider if visible
        if(selectedRightLED >= 0) {
          document.getElementById('rightLEDBrightnessSlider').value = value;
          document.getElementById('rightLEDBrightnessValue').textContent = value;
        }
      }
      
      const sideNum = side === 'left' ? 0 : 1;
      fetch(`/setMasterBrightness?side=${sideNum}&val=${value}`)
        .catch(err => console.error('Master brightness error:', err));
    }
    
    function resetSide(side) {
      const sideNum = side === 'left' ? 0 : 1;
      
      fetch(`/resetSide?side=${sideNum}`)
        .then(r => r.json())
        .then(data => {
          // Turn off all LEDs and reset brightness to master value
          const masterBrightness = side === 'left' ? leftMasterBrightness : rightMasterBrightness;
          for(let i = 0; i < NUM_LEDS; i++) {
            if(side === 'left') {
              leftLEDStates[i] = false;
              leftLEDBrightness[i] = masterBrightness;
            } else {
              rightLEDStates[i] = false;
              rightLEDBrightness[i] = masterBrightness;
            }
            updateLEDDisplay(side, i);
          }
          
          // Deselect any selected LED
          if(side === 'left' && selectedLeftLED >= 0) {
            const led = document.getElementById('left_led_' + selectedLeftLED);
            if(led) led.classList.remove('selected');
            selectedLeftLED = -1;
            document.getElementById('leftIndividualControl').classList.add('disabled');
            document.getElementById('leftIndividualTitle').textContent = 'Select an LED';
          } else if(side === 'right' && selectedRightLED >= 0) {
          const led = document.getElementById('right_led_' + selectedRightLED);
          if(led) led.classList.remove('selected');
            selectedRightLED = -1;
            document.getElementById('rightIndividualControl').classList.add('disabled');
            document.getElementById('rightIndividualTitle').textContent = 'Select an LED';
          }
        })
        .catch(err => console.error('Reset error:', err));
    }
    
    function toggleAllOn(side) {
      const sideNum = side === 'left' ? 0 : 1;
      
      fetch(`/toggleAllOn?side=${sideNum}`)
        .then(r => r.json())
        .then(data => {
          for(let i = 0; i < NUM_LEDS; i++) {
            if(side === 'left') {
              leftLEDStates[i] = true;
            } else {
              rightLEDStates[i] = true;
            }
            updateLEDDisplay(side, i);
          }
        })
        .catch(err => console.error('Toggle all error:', err));
    }
    
    function syncState() {
      fetch('/getState')
        .then(r => r.json())
        .then(data => {
          leftMasterBrightness = data.leftMasterBrightness;
          rightMasterBrightness = data.rightMasterBrightness;
          
          document.getElementById('leftMasterBrightnessValue').textContent = leftMasterBrightness;
          document.getElementById('leftMasterBrightnessSlider').value = leftMasterBrightness;
          document.getElementById('rightMasterBrightnessValue').textContent = rightMasterBrightness;
          document.getElementById('rightMasterBrightnessSlider').value = rightMasterBrightness;
          
          // Update left LEDs (0-46)
          for(let i = 0; i < NUM_LEDS; i++) {
            leftLEDStates[i] = data.ledStates[i];
            leftLEDBrightness[i] = data.ledBrightness[i];
            updateLEDDisplay('left', i);
          }
          
          // Update right LEDs (47-93)
          for(let i = 0; i < NUM_LEDS; i++) {
            rightLEDStates[i] = data.ledStates[i + NUM_LEDS];
            rightLEDBrightness[i] = data.ledBrightness[i + NUM_LEDS];
            updateLEDDisplay('right', i);
          }
          
          // Update selected LED sliders if any
          if(selectedLeftLED >= 0) {
            document.getElementById('leftLEDBrightnessSlider').value = leftLEDBrightness[selectedLeftLED];
            document.getElementById('leftLEDBrightnessValue').textContent = leftLEDBrightness[selectedLeftLED];
          }
          if(selectedRightLED >= 0) {
            document.getElementById('rightLEDBrightnessSlider').value = rightLEDBrightness[selectedRightLED];
            document.getElementById('rightLEDBrightnessValue').textContent = rightLEDBrightness[selectedRightLED];
          }
        })
        .catch(err => console.error('Sync error:', err));
    }
    
    createLEDCircle('leftLEDContainer', 'left');
    createLEDCircle('rightLEDContainer', 'right');
    
    syncState();
    setInterval(syncState, 1000);
  </script>
</body>
</html>
)rawliteral";
  
  server.send(200, "text/html", html);
}

void handleResetSide() {
  if(server.hasArg("side")) {
    int side = server.arg("side").toInt();
    if(side >= 0 && side <= 1) {
      int startIdx = side * NUM_LEDS_PER_STRIP;
      
      // Reset all LEDs on this side: turn off and reset brightness to master value
      for(int i = startIdx; i < startIdx + NUM_LEDS_PER_STRIP; i++) {
        ledStates[i] = false;
        if(side == 0) {
          ledBrightness[i] = leftMasterBrightness;
        } else {
          ledBrightness[i] = rightMasterBrightness;
        }
      }
      
      server.send(200, "application/json", "{\"success\":true}");
      return;
    }
  }
  server.send(400, "application/json", "{\"error\":\"Invalid side\"}");
}

void handleToggleAllOn() {
  if(server.hasArg("side")) {
    int side = server.arg("side").toInt();
    if(side >= 0 && side <= 1) {
      int startIdx = side * NUM_LEDS_PER_STRIP;
      bool newState = false;
      
      // Check if any LED is off on this side
      for(int i = startIdx; i < startIdx + NUM_LEDS_PER_STRIP; i++) {
        if(!ledStates[i]) {
          newState = true;
          break;
        }
      }
      
      // Toggle all LEDs on this side
      for(int i = startIdx; i < startIdx + NUM_LEDS_PER_STRIP; i++) {
        ledStates[i] = newState;
      }
      
      server.send(200, "application/json", 
        "{\"state\":" + String(newState ? "true" : "false") + "}");
      return;
    }
  }
  server.send(400, "application/json", "{\"error\":\"Invalid side\"}");
}

void handleToggleLED() {
  if(server.hasArg("led")) {
    int ledIndex = server.arg("led").toInt();
    if(ledIndex >= 0 && ledIndex < TOTAL_LEDS) {
      ledStates[ledIndex] = !ledStates[ledIndex];
      server.send(200, "application/json", 
        "{\"state\":" + String(ledStates[ledIndex] ? "true" : "false") + "}");
      return;
    }
  }
  server.send(400, "application/json", "{\"error\":\"Invalid LED\"}");
}

void handleSetMasterBrightness() {
  if(server.hasArg("side") && server.hasArg("val")) {
    int side = server.arg("side").toInt();
    int brightness = server.arg("val").toInt();
    
    if(side >= 0 && side <= 1 && brightness >= 0 && brightness <= 255) {
      int startIdx = side * NUM_LEDS_PER_STRIP;
      
      if(side == 0) {
        leftMasterBrightness = brightness;
        // Update all left LED brightness values to match master
        for(int i = startIdx; i < startIdx + NUM_LEDS_PER_STRIP; i++) {
          ledBrightness[i] = brightness;
        }
      } else {
        rightMasterBrightness = brightness;
        // Update all right LED brightness values to match master
        for(int i = startIdx; i < startIdx + NUM_LEDS_PER_STRIP; i++) {
          ledBrightness[i] = brightness;
        }
      }
      server.send(200, "application/json", "{\"success\":true}");
      return;
    }
  }
  server.send(400, "application/json", "{\"error\":\"Invalid parameters\"}");
}

void handleSetLEDBrightness() {
  if(server.hasArg("led") && server.hasArg("val")) {
    int ledIndex = server.arg("led").toInt();
    int brightness = server.arg("val").toInt();
    
    if(ledIndex >= 0 && ledIndex < TOTAL_LEDS && brightness >= 0 && brightness <= 255) {
      ledBrightness[ledIndex] = brightness;
      server.send(200, "application/json", "{\"success\":true}");
      return;
    }
  }
  server.send(400, "application/json", "{\"error\":\"Invalid parameters\"}");
}

void handleGetState() {
  String json = "{";
  json += "\"leftMasterBrightness\":" + String(leftMasterBrightness) + ",";
  json += "\"rightMasterBrightness\":" + String(rightMasterBrightness) + ",";
  json += "\"ledStates\":[";
  for(int i = 0; i < TOTAL_LEDS; i++) {
    json += ledStates[i] ? "true" : "false";
    if(i < TOTAL_LEDS - 1) json += ",";
  }
  json += "],";
  json += "\"ledBrightness\":[";
  for(int i = 0; i < TOTAL_LEDS; i++) {
    json += String(ledBrightness[i]);
    if(i < TOTAL_LEDS - 1) json += ",";
  }
  json += "]}";
  
  server.send(200, "application/json", json);
}