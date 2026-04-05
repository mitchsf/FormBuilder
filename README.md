# FormBuilder Library for ESP32

A comprehensive Arduino library for creating responsive HTML forms on ESP32 devices. Build web interfaces for device configuration with minimal code. The form is served from a `WiFiServer`, submitted via AJAX, and parsed back through a per-field callback. Saving data and reconnecting WiFi after AP teardown are left to the application — this library is strictly a form builder.

This library makes extensive use of Strings, but it is typically run only during initial configuration or when settings change. Stop calling `handleClient()` once form updates are complete. Call `cleanup()` to free memory.

This library was mostly written and documented by Anthropic Claude Sonnet 4.6 Extended.

## Features

- Responsive, mobile-friendly design with modern CSS3 styling
- AJAX form submission (no page refresh)
- Change detection — callback receives a `valueChanged` flag per field
- Form-complete callback for post-processing (NVS write, reboot, etc.)
- Custom CSS injection
- `cleanup()` to release server, callbacks, and String memory when done

## Field Types

| Method | Description | Callback value |
|--------|-------------|----------------|
| `addText(prompt, default)` | Single-line text input | String |
| `addPassword(prompt, default)` | Password input with show/hide toggle | String |
| `addDropDown(prompt, options, index, returnText)` | Select from comma-separated options | Index (int) or option text |
| `addDropDownRange(prompt, min, max, default)` | Numeric range dropdown (e.g. 0–23) | Int as string |
| `addNumber(prompt, min, max, step, default)` | Validated number input | Int as string |
| `addRange(prompt, min, max, step, default)` | Slider with live value display | Int as string |
| `addColorPicker(prompt, defaultColor)` | Visual color picker | Int (decimal, from 0xRRGGBB) |
| `addTime(prompt, defaultTime, includeSeconds)` | Time picker (HH:MM or HH:MM:SS) | Int (HHMM, e.g. 1356) |
| `addCheckbox(prompt, defaultChecked)` | Checkbox | `"true"` / `"false"` |
| `addRadio(prompt, options, index, returnText)` | Radio button group | Index (int) or option text |
| `addHidden(defaultValue)` | Invisible field — preserves index numbering | String (unchanged) |
| `addSubheading(text)` | Section header (no field index consumed) | — |

## Quick Start

```cpp
#include <WiFi.h>
#include "FormBuilder.h"

WiFiServer server(80);
FormBuilder form;

void buildForm() {
    form.addSubheading("Network Settings");
    form.addText("Device Name", "ESP32-Device");
    form.addPassword("WiFi Password", "");
    form.addDropDown("WiFi Mode", "Station,Access Point", 0, true);

    form.addSubheading("Display Settings");
    form.addColorPicker("Theme Color", 0x2563EB);
    form.addRange("Brightness", 0, 100, 1, 75);
    form.addTime("Auto-Off Time", 2300);
    form.addCheckbox("Enable Sleep Mode", true);
}

void handleFormData(int fieldIndex, String value, bool valueChanged) {
    // fieldIndex is 1-based, matching addXxx() call order
    // valueChanged is true if user altered the default
    switch (fieldIndex) {
        case 1: Serial.println("Name: " + value); break;
        case 2: if (valueChanged) Serial.println("New password set"); break;
        case 3: Serial.println("WiFi Mode: " + value); break;
        case 4: Serial.println("Color: " + value); break;
        case 5: Serial.println("Brightness: " + value); break;
        case 6: Serial.println("Auto-Off: " + value); break;
        case 7: Serial.println("Sleep: " + value); break;
    }
}

void onFormComplete() {
    // All fields processed — write to NVS, reboot, etc.
    Serial.println("Configuration saved.");
}

void setup() {
    Serial.begin(115200);
    WiFi.begin("ssid", "pass");
    while (WiFi.status() != WL_CONNECTED) delay(500);

    form.begin(&server);
    form.setTitle("Device Configuration");
    form.setFormBuilder(buildForm);
    form.setCallback(handleFormData);
    form.setFormCompleteCallback(onFormComplete);

    server.begin();
}

void loop() {
    form.handleClient();
}
```

## Installation

Copy `FormBuilder.h` and `FormBuilder.cpp` into your project's `src/` or `lib/` directory.

### Arduino Library Structure

```
FormBuilder/
├── src/
│   ├── FormBuilder.h
│   └── FormBuilder.cpp
├── examples/
│   └── BasicForm/
│       └── BasicForm.ino
├── library.properties
├── keywords.txt
├── README.md
└── LICENSE
```

## API Reference

### Setup & Lifecycle

| Method | Description |
|--------|-------------|
| `begin(WiFiServer*)` | Attach to a WiFi server |
| `setTitle(title)` | Set page title and header text |
| `addCustomCSS(css)` | Inject additional CSS rules into the page |
| `setFormBuilder(cb)` | Register the function that calls `addXxx()` to build the form |
| `setCallback(cb)` | Register the per-field data callback: `void cb(int fieldIndex, String value, bool valueChanged)` |
| `setFormCompleteCallback(cb)` | Called once after all fields have been processed |
| `handleClient()` | Process incoming HTTP requests — call from `loop()` |
| `cleanup()` | Stop server, clear callbacks and Strings, free memory |

### Field Builders

All field methods consume a sequential 1-based field index (except `addSubheading`, which does not).

| Method | Signature |
|--------|-----------|
| `addText` | `(String prompt, String defaultValue)` |
| `addPassword` | `(String prompt, String defaultValue)` |
| `addDropDown` | `(String prompt, String options, int defaultIndex, bool returnText = false)` |
| `addDropDownRange` | `(String prompt, int minVal, int maxVal, int defaultValue)` |
| `addNumber` | `(String prompt, int minVal, int maxVal, int step, int defaultValue)` |
| `addRange` | `(String prompt, int minVal, int maxVal, int step, int defaultValue)` |
| `addColorPicker` | `(String prompt, int defaultColor)` |
| `addTime` | `(String prompt, int defaultTime, bool includeSeconds = false)` |
| `addCheckbox` | `(String prompt, bool defaultChecked)` |
| `addRadio` | `(String prompt, String options, int defaultIndex, bool returnText = false)` |
| `addHidden` | `(String defaultValue)` |
| `addSubheading` | `(String text)` — visual only, no field index |

### Compile-Time Limits

Override before including the header if needed:

```cpp
#define MAX_FORM_FIELDS  100   // maximum number of form fields
#define MAX_FIELD_OPTIONS 50   // maximum options per dropdown/radio
#define MAX_VALID         10   // maximum valid-value entries per field
```

## Color Handling

Color pickers accept and return 24-bit integers in 0xRRGGBB format:

```cpp
form.addColorPicker("Accent", 0xFF8800);   // orange default

// In callback, value arrives as a decimal string:
int color = value.toInt();                  // e.g. 16744448
```

## Time Handling

Time fields use integer encoding — `HHMM` (e.g., `1356` = 13:56):

```cpp
form.addTime("Wake Time", 630);            // 06:30
form.addTime("Precise Time", 1200, true);  // 12:00:00 with seconds

// In callback, value arrives as HHMM int string:
int t = value.toInt();                      // 630
int hours = t / 100;                        // 6
int minutes = t % 100;                      // 30
```

## Hidden Fields

Use `addHidden()` to hold a field index slot without rendering anything visible. Useful when a preset array index is reserved but has no user-facing control:

```cpp
form.addText("Name", "Clock");      // field 1
form.addHidden("0");                // field 2 — placeholder
form.addDropDown("Mode", "A,B", 0); // field 3
```

## Compatibility

- **Platform**: ESP32 (Arduino framework)
- **Arduino Core**: ESP32 Arduino Core 2.0+
- **Dependencies**: WiFi (included with ESP32 core)
- **Browsers**: Chrome 60+, Firefox 55+, Safari 12+, Edge 79+

## License

MIT
