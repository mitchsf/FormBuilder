# FormBuilder Library for ESP32

A comprehensive Arduino library for creating responsive HTML forms on ESP32 devices. Build beautiful web interfaces for device configuration with minimal code. This makes extensive use of
the Strings, but it is typically run on initial configuration, and when settings are changed. Don't run the update method in loop(), once form updates are complete. This library leaves
saveing data and connecting to WiFi after AP has closed, to the application. It is strictly a form builder.

## Features

- **Responsive Design**: Modern, mobile-friendly forms that look great on all devices
- **Multiple Field Types**: Text inputs, dropdowns, range selectors, color pickers, and subheadings
- **Easy Integration**: Simple callback-based architecture
- **Modern UI**: Clean, professional styling with CSS3 animations
- **AJAX Submission**: Seamless form submission without page refresh
- **Auto-restart**: Configurable device restart after form submission

## Field Types

- **Text Input**: Single-line text fields with default values
- **Dropdown**: Select from predefined options with text or index return values
- **Range Dropdown**: Numeric ranges (e.g., 0-23 for hours)
- **Color Picker**: Visual color selection with hex/integer conversion
- **Subheadings**: Organize form sections with styled headers

## Quick Start

```cpp
#include <WiFi.h>
#include "FormBuilder.h"

WiFiServer server(80);
FormBuilder form;

void setup() {
    Serial.begin(115200);
    
    // Connect to WiFi
    WiFi.begin("your-wifi", "your-password");
    while (WiFi.status() != WL_CONNECTED) {
        delay(1000);
        Serial.println("Connecting to WiFi...");
    }
    
    // Setup form
    form.begin(&server);
    form.setTitle("Device Configuration");
    form.setFormBuilder(buildForm);
    form.setCallback(handleFormData);
    
    server.begin();
    Serial.println("Form server started at: " + WiFi.localIP().toString());
}

void loop() {
    form.handleClient();
}

void buildForm() {
    form.addSubheading("Network Settings");
    form.addText("Device Name", "ESP32-Device");
    form.addDropDown("WiFi Mode", "Station,Access Point", 0, true);
    
    form.addSubheading("Display Settings");
    form.addColorPicker("Theme Color", 0x2563eb);
    form.addDropDownRange("Brightness", 0, 100, 75);
}

void handleFormData(int fieldIndex, String value) {
    switch(fieldIndex) {
        case 1: Serial.println("Device Name: " + value); break;
        case 2: Serial.println("WiFi Mode: " + value); break;
        case 3: Serial.println("Theme Color: " + value); break;
        case 4: Serial.println("Brightness: " + value); break;
    }
}
```

## Installation

### Arduino IDE

1. Download or copy all library files
2. Create a folder named `FormBuilder` in your Arduino libraries directory
3. Copy files to the proper structure (see below)
4. Restart Arduino IDE

### Library Structure

```
FormBuilder/
├── src/
│   ├── FormBuilder.h
│   └── FormBuilder.cpp
├── examples/
│   ├── FormBuilder/
│   │   └── FormBuilder.ino
│   ├── BasicForm/
│   │   └── BasicForm.ino
│   └── AdvancedForm/
│       └── AdvancedForm.ino
├── library.properties
├── keywords.txt
├── README.md
└── LICENSE
```

## API Reference

### Core Methods

- `FormBuilder()` - Constructor
- `void begin(WiFiServer* server)` - Initialize with WiFi server
- `void setTitle(String title)` - Set page title
- `void setFormBuilder(FormBuilderCallback callback)` - Set form builder function
- `void setCallback(FormDataCallback callback)` - Set data handler function
- `void handleClient()` - Process HTTP requests (call in loop)

### Field Methods

- `void addText(String prompt, String defaultValue)` - Add text input
- `void addDropDown(String prompt, String options, int defaultIndex, bool returnText)` - Add dropdown
- `void addDropDownRange(String prompt, int minVal, int maxVal, int defaultValue)` - Add range dropdown
- `void addColorPicker(String prompt, int defaultColor)` - Add color picker
- `void addSubheading(String text)` - Add section heading

## Examples

The library includes three complete examples:

1. **FormBuilder.ino** - Comprehensive test with all field types
2. **BasicForm.ino** - Simple configuration form
3. **AdvancedForm.ino** - Complex form with EEPROM storage

## Color Handling

Color pickers use 24-bit integers (0xRRGGBB format):

```cpp
form.addColorPicker("Theme Color", 0xFF0000);  // Red
form.addColorPicker("Background", 0x2563eb);   // Blue

// In callback, convert back:
int colorValue = value.toInt();
int red = (colorValue >> 16) & 0xFF;
int green = (colorValue >> 8) & 0xFF;
int blue = colorValue & 0xFF;
```

## Compatibility

- **Platform**: ESP32
- **Arduino Core**: ESP32 Arduino Core 2.0+
- **Dependencies**: WiFi library (included)
- **Browsers**: Chrome 60+, Firefox 55+, Safari 12+, Edge 79+

## License

MIT License - see LICENSE file for details.
