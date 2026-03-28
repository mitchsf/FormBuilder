/**
 * FormBuilder.cpp - Generic HTML Form Builder Library for ESP32
 * 
 * Implementation of the FormBuilder class for creating responsive web forms.
 */

#include "FormBuilder.h"

/**
 * Constructor
 */
FormBuilder::FormBuilder() {
    _server = nullptr;
    _callback = nullptr;
    _formBuilderCallback = nullptr;
    _formCompleteCallback = nullptr;
    _fieldTag = START_FIELD_TAG;
    _numberFields = 0;
    _pageTitle = "Default Title";
    _customCSS = "";
    
    // Initialize default values array
    for (int i = 0; i < MAX_FORM_FIELDS; i++) {
        _fieldDefaults[i] = "";
    }
    
    clearSettings();
}

/**
 * Initialize the form builder with a WiFi server
 */
void FormBuilder::begin(WiFiServer* server) {
    _server = server;
}

/**
 * Set the callback function for form data processing
 */
void FormBuilder::setCallback(FormDataCallback callback) {
    _callback = callback;
}

/**
 * Set the callback function for building the form
 */
void FormBuilder::setFormBuilder(FormBuilderCallback callback) {
    _formBuilderCallback = callback;
}

/**
 * Set the callback function for when all form processing is complete
 */
void FormBuilder::setFormCompleteCallback(FormCompleteCallback callback) {
    _formCompleteCallback = callback;
}

/**
 * Handle incoming client connections and form submissions
 */
void FormBuilder::handleClient() {
    if (!_server) return;
    if (!_server->hasClient()) return;
    
    _client = _server->accept();
    if (_client) {
        unsigned long waitStart = millis();
        while (!_client.available() && _client.connected()) {
            if (millis() - waitStart > 2000) break;
            yield();
        }
        if (_client.available()) {
            getParameters();
        } else {
            _client.stop();
        }
    }
}

/**
 * Clean up and free resources when form functionality no longer needed
 */
void FormBuilder::cleanup() {
    // Stop the server and clear client
    if (_server) {
        _server->stop();
        _server = nullptr;
    }
    
    if (_client) {
        _client.stop();
    }
    
    // Clear callbacks
    _callback = nullptr;
    _formBuilderCallback = nullptr;
    _formCompleteCallback = nullptr;
    
    // Clear all settings and strings
    clearSettings();
    _pageTitle = "";
    
    // Reset counters
    _fieldTag = START_FIELD_TAG;
    _numberFields = 0;
    
    // Clear default values
    for (int i = 0; i < MAX_FORM_FIELDS; i++) {
        _fieldDefaults[i] = "";
    }
    
    // Force garbage collection of any String objects
    _settings.fieldPrompt = String();
    _settings.textDefault = String();
    for (byte x = 0; x < MAX_FIELD_OPTIONS; x++) {
        _settings.fieldOptions[x] = String();
    }
}

/**
 * Set the page title displayed in browser tab and header
 */
void FormBuilder::setTitle(String title) {
    _pageTitle = title;
}

/**
 * Add custom CSS to be injected into the page
 */
void FormBuilder::addCustomCSS(String css) {
    _customCSS = css;
}

/**
 * Add a subheading to organize form sections
 */
void FormBuilder::addSubheading(String text) {
    renderSubheading(text);
}

/**
 * Add a text input field to the form
 */
void FormBuilder::addText(String prompt, String defaultValue) {
    clearSettings();
    _settings.fieldPrompt = prompt;
    _settings.textDefault = defaultValue;
    renderTextInput();
}

/**
 * Add a dropdown field with comma-separated options
 */
void FormBuilder::addDropDown(String prompt, String options, int defaultIndex, bool returnText) {
    clearSettings();
    _settings.fieldPrompt = prompt;
    
    // Parse comma-separated options
    int optionCount = 0;
    int lastComma = -1;
    int nextComma = 0;
    
    while (nextComma != -1 && optionCount < MAX_FIELD_OPTIONS) {
        nextComma = options.indexOf(',', lastComma + 1);
        String option;
        if (nextComma == -1) {
            option = options.substring(lastComma + 1);
        } else {
            option = options.substring(lastComma + 1, nextComma);
        }
        option.trim();
        _settings.fieldOptions[optionCount] = option;
        optionCount++;
        lastComma = nextComma;
    }
    
    _settings.numDefault = defaultIndex;
    _settings.isRangeDropdown = false;
    _settings.returnPrompts = returnText;
    
    renderDropdown();
}

/**
 * Add a range dropdown (e.g., 0-23 for hours)
 */
void FormBuilder::addDropDownRange(String prompt, int minVal, int maxVal, int defaultValue) {
    clearSettings();
    _settings.fieldPrompt = prompt;
    _settings.rangeMin = minVal;
    _settings.rangeMax = maxVal;
    _settings.rangeDefault = defaultValue;
    _settings.isRangeDropdown = true;
    _settings.returnPrompts = false;
    
    // Set valid array for compatibility
    _settings.valid[0] = minVal;
    _settings.valid[1] = maxVal;
    
    renderDropdown();
}

/**
 * Add a color picker field
 */
void FormBuilder::addColorPicker(String prompt, int defaultColor) {
    clearSettings();
    _settings.fieldPrompt = prompt;
    _settings.colorDefault = defaultColor;
    
    // Convert int color to hex string for HTML input
    String hex = "#";
    if (defaultColor < 0x100000) hex += "0";
    if (defaultColor < 0x10000) hex += "0";
    if (defaultColor < 0x1000) hex += "0";
    if (defaultColor < 0x100) hex += "0";
    if (defaultColor < 0x10) hex += "0";
    hex += String(defaultColor, HEX);
    hex.toUpperCase();
    _settings.textDefault = hex;
    
    _settings.isColorPicker = true;
    
    renderColorPicker();
}

/**
 * Add a number input field with range validation
 */
void FormBuilder::addNumber(String prompt, int minVal, int maxVal, int step, int defaultValue) {
    clearSettings();
    _settings.fieldPrompt = prompt;
    _settings.isNumberInput = true;
    _settings.numberMin = minVal;
    _settings.numberMax = maxVal;
    _settings.numberStep = step;
    _settings.numberDefault = defaultValue;
    
    renderNumberInput();
}

/**
 * Add a range slider for numeric values
 */
void FormBuilder::addRange(String prompt, int minVal, int maxVal, int step, int defaultValue) {
    clearSettings();
    _settings.fieldPrompt = prompt;
    _settings.isRangeSlider = true;
    _settings.rangeMin = minVal;
    _settings.rangeMax = maxVal;
    _settings.rangeStep = step;
    _settings.rangeDefault = defaultValue;
    
    renderRangeSlider();
}

/**
 * Add a time picker input
 */
void FormBuilder::addTime(String prompt, int defaultTime, bool includeSeconds) {
    clearSettings();
    _settings.fieldPrompt = prompt;
    _settings.isTimeInput = true;
    
    // Convert integer time to HH:MM format
    int hours = defaultTime / 100;
    int minutes = defaultTime % 100;
    
    // Format with leading zeros
    String timeString = "";
    if (hours < 10) timeString += "0";
    timeString += String(hours);
    timeString += ":";
    if (minutes < 10) timeString += "0";
    timeString += String(minutes);
    
    _settings.textDefault = timeString;
    _settings.timeIncludeSeconds = includeSeconds;
    
    renderTimeInput();
}

/**
 * Add a password input field
 */
void FormBuilder::addPassword(String prompt, String defaultValue) {
    clearSettings();
    _settings.fieldPrompt = prompt;
    _settings.textDefault = defaultValue;
    _settings.isPasswordInput = true;
    
    renderPasswordInput();
}

/**
 * Add a checkbox input
 */
void FormBuilder::addCheckbox(String prompt, bool defaultChecked) {
    clearSettings();
    _settings.fieldPrompt = prompt;
    _settings.isCheckbox = true;
    _settings.checkboxDefault = defaultChecked;
    
    renderCheckbox();
}

/**
 * Add a radio button group with comma-separated options
 */
void FormBuilder::addRadio(String prompt, String options, int defaultIndex, bool returnText) {
    clearSettings();
    _settings.fieldPrompt = prompt;
    
    // Parse comma-separated options
    int optionCount = 0;
    int lastComma = -1;
    int nextComma = 0;
    
    while (nextComma != -1 && optionCount < MAX_FIELD_OPTIONS) {
        nextComma = options.indexOf(',', lastComma + 1);
        String option;
        if (nextComma == -1) {
            option = options.substring(lastComma + 1);
        } else {
            option = options.substring(lastComma + 1, nextComma);
        }
        option.trim();
        _settings.fieldOptions[optionCount] = option;
        optionCount++;
        lastComma = nextComma;
    }
    
    _settings.numDefault = defaultIndex;
    _settings.isRadio = true;
    _settings.returnPrompts = returnText;
    
    renderRadio();
}

/**
 * Add a hidden form field (invisible, preserves field numbering)
 */
void FormBuilder::addHidden(String defaultValue) {
    clearSettings();
    _settings.textDefault = defaultValue;
    renderHidden();
}

/**
 * Clear all settings variables
 */
void FormBuilder::clearSettings() {
    _settings.fieldPrompt = "";
    _settings.textDefault = "";
    _settings.numDefault = 0;
    _settings.heading = "";
    for (byte x = 0; x < MAX_FIELD_OPTIONS; x++) _settings.fieldOptions[x] = "";
    for (byte x = 0; x < MAX_VALID; x++) _settings.valid[x] = 0;
    _settings.returnPrompts = false;
    _settings.returnVal = "";
    _settings.isRangeDropdown = false;
    _settings.isColorPicker = false;
    _settings.isNumberInput = false;
    _settings.numberMin = 0;
    _settings.numberMax = 100;
    _settings.numberStep = 1;
    _settings.numberDefault = 0;
    _settings.isRangeSlider = false;
    _settings.rangeStep = 1;
    _settings.isTimeInput = false;
    _settings.timeIncludeSeconds = false;
    _settings.isPasswordInput = false;
    _settings.isCheckbox = false;
    _settings.checkboxDefault = false;
    _settings.isRadio = false;
}

/**
 * Render subheading to HTML form
 */
void FormBuilder::renderSubheading(String text) {
    _client.println("<h2 class=\"subheading\">" + text + "</h2>");
}

/**
 * Render dropdown field to HTML form
 */
void FormBuilder::renderDropdown() {
    if (_settings.fieldPrompt == "") return;
    
    _fieldTag++;
    _numberFields++;
    String fieldId = "x" + String(_fieldTag);

    // Store default value for change detection
    if (_settings.isRangeDropdown) {
        _fieldDefaults[_numberFields - 1] = String(_settings.rangeDefault);
    } else {
        // Store the default option value based on returnPrompts setting
        if (_settings.numDefault < MAX_FIELD_OPTIONS && _settings.fieldOptions[_settings.numDefault] != "") {
            _fieldDefaults[_numberFields - 1] = _settings.returnPrompts ? 
                _settings.fieldOptions[_settings.numDefault] : String(_settings.numDefault);
        }
    }

    if (_settings.heading != "") _client.println("<h2>" + _settings.heading + "</h2>");

    // Create field group container
    _client.println("<div class=\"field-group\">");
    _client.println("<label class=\"field-label\">" + _settings.fieldPrompt + "</label>");
    _client.println("<select id=\"" + fieldId + "\">");

    if (_settings.isRangeDropdown) {
        // Generate range options
        for (int option = _settings.rangeMin; option <= _settings.rangeMax; option++) {
            String sel = (option == _settings.rangeDefault) ? " selected" : "";
            _client.println("<option value=\"" + String(option) + "\"" + sel + ">" + String(option) + "</option>");
        }
    } else {
        // Use predefined options
        for (int option = 0; option < MAX_FIELD_OPTIONS; option++) {
            if (_settings.fieldOptions[option] == "") break;
            if (_settings.fieldOptions[option].length() == 0) continue;
            
            // Use actual text as value if returnPrompts is true, otherwise use index
            String optVal = _settings.returnPrompts ? _settings.fieldOptions[option] : String(option);
            String optText = _settings.fieldOptions[option];
            String sel = (option == _settings.numDefault) ? " selected" : "";
            _client.println("<option value=\"" + optVal + "\"" + sel + ">" + optText + "</option>");
        }
    }

    _client.println("</select>");
    _client.println("</div>");
}

/**
 * Render text input field to HTML form
 */
void FormBuilder::renderTextInput() {
    if (_settings.fieldPrompt == "") return;
    
    _fieldTag++;
    _numberFields++;
    String fieldId = "x" + String(_fieldTag);

    // Store default value for change detection
    _fieldDefaults[_numberFields - 1] = _settings.textDefault;

    if (_settings.heading != "") _client.println("<h2>" + _settings.heading + "</h2>");

    // Create field group container
    _client.println("<div class=\"field-group\">");
    _client.println("<label class=\"field-label\">" + _settings.fieldPrompt + "</label>");
    _client.println("<input type='text' id='" + fieldId + "' value='" + _settings.textDefault + "'>");
    _client.println("</div>");
}

/**
 * Render color picker field to HTML form
 */
void FormBuilder::renderColorPicker() {
    if (_settings.fieldPrompt == "") return;
    
    _fieldTag++;
    _numberFields++;
    String fieldId = "x" + String(_fieldTag);

    // Store default value for change detection (as integer string)
    _fieldDefaults[_numberFields - 1] = String(_settings.colorDefault);

    if (_settings.heading != "") _client.println("<h2>" + _settings.heading + "</h2>");

    // Create field group container
    _client.println("<div class=\"field-group\">");
    _client.println("<label class=\"field-label\">" + _settings.fieldPrompt + "</label>");
    _client.println("<input type='color' id='" + fieldId + "' value='" + _settings.textDefault + "'>");
    _client.println("</div>");
}

/**
 * Render number input field to HTML form
 */
void FormBuilder::renderNumberInput() {
    if (_settings.fieldPrompt == "") return;
    
    _fieldTag++;
    _numberFields++;
    String fieldId = "x" + String(_fieldTag);

    // Store default value for change detection
    _fieldDefaults[_numberFields - 1] = String(_settings.numberDefault);

    if (_settings.heading != "") _client.println("<h2>" + _settings.heading + "</h2>");

    // Create field group container
    _client.println("<div class=\"field-group\">");
    _client.println("<label class=\"field-label\">" + _settings.fieldPrompt + "</label>");
    _client.println("<input type='number' id='" + fieldId + "' min='" + String(_settings.numberMin) + 
                    "' max='" + String(_settings.numberMax) + "' step='" + String(_settings.numberStep) + 
                    "' value='" + String(_settings.numberDefault) + "'>");
    _client.println("</div>");
}

/**
 * Render range slider to HTML form
 */
void FormBuilder::renderRangeSlider() {
    if (_settings.fieldPrompt == "") return;
    
    _fieldTag++;
    _numberFields++;
    String fieldId = "x" + String(_fieldTag);

    // Store default value for change detection
    _fieldDefaults[_numberFields - 1] = String(_settings.rangeDefault);

    if (_settings.heading != "") _client.println("<h2>" + _settings.heading + "</h2>");

    // Create field group container
    _client.println("<div class=\"field-group\">");
    _client.println("<label class=\"field-label\">" + _settings.fieldPrompt + "</label>");
    _client.println("<div class=\"range-container\">");
    _client.println("<input type='range' id='" + fieldId + "' min='" + String(_settings.rangeMin) + 
                    "' max='" + String(_settings.rangeMax) + "' step='" + String(_settings.rangeStep) + 
                    "' value='" + String(_settings.rangeDefault) + "' oninput='updateRangeValue(\"" + 
                    fieldId + "\", this.value)'>");
    _client.println("<span class=\"range-value\" id='" + fieldId + "_value'>" + String(_settings.rangeDefault) + "</span>");
    _client.println("</div>");
    _client.println("</div>");
}

/**
 * Render time input field to HTML form
 */
void FormBuilder::renderTimeInput() {
    if (_settings.fieldPrompt == "") return;
    
    _fieldTag++;
    _numberFields++;
    String fieldId = "x" + String(_fieldTag);

    // Store default value for change detection (as integer for callback comparison)
    int hours = _settings.textDefault.substring(0, 2).toInt();
    int minutes = _settings.textDefault.substring(3, 5).toInt();
    int timeInt = hours * 100 + minutes;
    _fieldDefaults[_numberFields - 1] = String(timeInt);

    if (_settings.heading != "") _client.println("<h2>" + _settings.heading + "</h2>");

    // Create field group container
    _client.println("<div class=\"field-group\">");
    _client.println("<label class=\"field-label\">" + _settings.fieldPrompt + "</label>");
    String stepAttr = _settings.timeIncludeSeconds ? " step='1'" : "";
    _client.println("<input type='time' id='" + fieldId + "' value='" + _settings.textDefault + "'" + stepAttr + ">");
    _client.println("</div>");
}

/**
 * Render password input field to HTML form
 */
void FormBuilder::renderPasswordInput() {
    if (_settings.fieldPrompt == "") return;
    
    _fieldTag++;
    _numberFields++;
    String fieldId = "x" + String(_fieldTag);

    // Store default value for change detection
    _fieldDefaults[_numberFields - 1] = _settings.textDefault;

    if (_settings.heading != "") _client.println("<h2>" + _settings.heading + "</h2>");

    // Create field group container
    _client.println("<div class=\"field-group\">");
    _client.println("<label class=\"field-label\">" + _settings.fieldPrompt + "</label>");
    _client.println("<div class=\"password-container\">");
    _client.println("<input type='password' id='" + fieldId + "' value='" + _settings.textDefault + "'>");
    _client.println("<label class=\"show-password-label\">");
    _client.println("<input type='checkbox' onclick='togglePassword(\"" + fieldId + "\")'>");
    _client.println("<span>Show</span>");
    _client.println("</label>");
    _client.println("</div>");
    _client.println("</div>");
}

/**
 * Render checkbox input to HTML form
 */
void FormBuilder::renderCheckbox() {
    if (_settings.fieldPrompt == "") return;
    
    _fieldTag++;
    _numberFields++;
    String fieldId = "x" + String(_fieldTag);

    // Store default value for change detection
    _fieldDefaults[_numberFields - 1] = _settings.checkboxDefault ? "true" : "false";

    if (_settings.heading != "") _client.println("<h2>" + _settings.heading + "</h2>");

    // Create field group container
    _client.println("<div class=\"field-group checkbox-group\">");
    String checked = _settings.checkboxDefault ? " checked" : "";
    _client.println("<label class=\"checkbox-label\">");
    _client.println("<input type='checkbox' id='" + fieldId + "' value='true'" + checked + ">");
    _client.println("<span class=\"checkbox-text\">" + _settings.fieldPrompt + "</span>");
    _client.println("</label>");
    _client.println("</div>");
}

/**
 * Render radio button group to HTML form
 */
void FormBuilder::renderRadio() {
    if (_settings.fieldPrompt == "") return;
    
    _fieldTag++;
    _numberFields++;
    String fieldId = "x" + String(_fieldTag);
    String groupName = "group_" + fieldId;

    // Store default value for change detection
    if (_settings.numDefault < MAX_FIELD_OPTIONS && _settings.fieldOptions[_settings.numDefault] != "") {
        _fieldDefaults[_numberFields - 1] = _settings.returnPrompts ? 
            _settings.fieldOptions[_settings.numDefault] : String(_settings.numDefault);
    }

    if (_settings.heading != "") _client.println("<h2>" + _settings.heading + "</h2>");

    // Create field group container
    _client.println("<div class=\"field-group\">");
    _client.println("<label class=\"field-label\">" + _settings.fieldPrompt + "</label>");
    
    // Generate radio buttons for each option
    for (int option = 0; option < MAX_FIELD_OPTIONS; option++) {
        if (_settings.fieldOptions[option] == "") break;
        if (_settings.fieldOptions[option].length() == 0) continue;
        
        // Use actual text as value if returnPrompts is true, otherwise use index
        String optVal = _settings.returnPrompts ? _settings.fieldOptions[option] : String(option);
        String optText = _settings.fieldOptions[option];
        String checked = (option == _settings.numDefault) ? " checked" : "";
        String radioId = fieldId + "_" + String(option);
        
        _client.println("<div class=\"radio-group\">");
        _client.println("<label class=\"radio-label\">");
        _client.println("<input type='radio' id='" + radioId + "' name='" + groupName + 
                        "' value='" + optVal + "'" + checked + ">");
        _client.println("<span class=\"radio-text\">" + optText + "</span>");
        _client.println("</label>");
        _client.println("</div>");
    }
    
    _client.println("</div>");
}

/**
 * Render hidden field — no visible HTML, just a hidden input to hold the slot
 */
void FormBuilder::renderHidden() {
    _fieldTag++;
    _numberFields++;
    String fieldId = "x" + String(_fieldTag);

    _fieldDefaults[_numberFields - 1] = _settings.textDefault;

    _client.println("<input type='hidden' id='" + fieldId + "' value='" + _settings.textDefault + "'>");
}

/**
 * Start HTML form output
 */
void FormBuilder::htmlStart() {
    // Reset form generation state for fresh form
    _fieldTag = START_FIELD_TAG;
    _numberFields = 0;
    
    // Clear default values for new form
    for (int i = 0; i < MAX_FORM_FIELDS; i++) {
        _fieldDefaults[i] = "";
    }
    
    _client.println("HTTP/1.1 200 OK");
    _client.println("Content-type:text/html");
    _client.println("Connection: close");
    _client.println();
    _client.println("<!DOCTYPE html>");
    _client.println("<html lang=\"en\">");
    _client.println("<head>");
    _client.println("<meta charset=\"UTF-8\">");
    _client.println("<meta name=\"viewport\" content=\"width=device-width, initial-scale=1.0\">");
    
    // Enhanced CSS with modern styling
    _client.println("<style>");
    _client.println(":root {");
    _client.println("  --primary-color: #2563eb;");
    _client.println("  --primary-hover: #1d4ed8;");
    _client.println("  --success-color: #059669;");
    _client.println("  --background: #f8fafc;");
    _client.println("  --card-bg: #ffffff;");
    _client.println("  --text-primary: #1e293b;");
    _client.println("  --text-secondary: #475569;");
    _client.println("  --border: #e2e8f0;");
    _client.println("  --border-focus: #3b82f6;");
    _client.println("  --shadow: 0 4px 6px -1px rgba(0, 0, 0, 0.1);");
    _client.println("  --shadow-lg: 0 10px 15px -3px rgba(0, 0, 0, 0.1);");
    _client.println("}");
    
    _client.println("* { box-sizing: border-box; }");
    
    _client.println("body {");
    _client.println("  font-family: -apple-system, BlinkMacSystemFont, 'Segoe UI', Roboto, sans-serif;");
    _client.println("  background: linear-gradient(135deg, var(--background) 0%, #e2e8f0 100%);");
    _client.println("  margin: 0; padding: 20px; color: var(--text-primary); line-height: 1.6;");
    _client.println("}");
    
    _client.println("#container {");
    _client.println("  max-width: 800px; margin: 0 auto; background: var(--card-bg);");
    _client.println("  border-radius: 16px; box-shadow: var(--shadow-lg); overflow: hidden;");
    _client.println("}");
    
    _client.println("#header {");
    _client.println("  background: linear-gradient(135deg, var(--primary-color) 0%, var(--primary-hover) 100%);");
    _client.println("  color: white; text-align: center; padding: 0px 20px; font-size: 1.1rem;");
    _client.println("  font-weight: 700; margin: 0; letter-spacing: -0.5px;");
    _client.println("  border-radius: 16px 16px 0 0; line-height: 0.9;");
    _client.println("}");
    
    _client.println("#inputs { padding: 40px; margin-top: 5px; }");
    
    _client.println(".subheading {");
    _client.println("  font-size: 1.5rem; font-weight: 600; color: var(--text-primary);");
    _client.println("  margin: 40px 0 20px 0; padding-bottom: 10px;");
    _client.println("  border-bottom: 2px solid var(--border);");
    _client.println("}");
    _client.println(".subheading:first-child { margin-top: 0; }");
    
    _client.println(".field-group { margin-bottom: 24px; overflow: hidden; }");
    
    _client.println(".field-label {");
    _client.println("  display: block; font-size: 1.1rem; font-weight: 500;");
    _client.println("  color: var(--text-primary); margin-bottom: 8px;");
    _client.println("}");
    
    _client.println("input[type=\"text\"], input[type=\"password\"], input[type=\"number\"], input[type=\"time\"], select {");
    _client.println("  width: 100%; height: 48px; padding: 12px; font-size: 1.1rem;");
    _client.println("  border: 2px solid var(--border); border-radius: 8px;");
    _client.println("  background: var(--card-bg); transition: all 0.2s ease; outline: none;");
    _client.println("}");
    
    _client.println("input[type=\"text\"]:focus, input[type=\"password\"]:focus, input[type=\"number\"]:focus, input[type=\"time\"]:focus, select:focus {");
    _client.println("  border-color: var(--border-focus);");
    _client.println("  box-shadow: 0 0 0 3px rgba(59, 130, 246, 0.1);");
    _client.println("}");
    
    // Password container styling
    _client.println(".password-container {");
    _client.println("  display: flex; align-items: center; gap: 12px;");
    _client.println("}");
    
    _client.println(".password-container input[type=\"password\"], .password-container input[type=\"text\"] {");
    _client.println("  flex: 1;");
    _client.println("}");
    
    _client.println(".show-password-label {");
    _client.println("  display: flex; align-items: center; cursor: pointer; font-size: 0.9rem;");
    _client.println("  color: var(--text-secondary); white-space: nowrap; margin: 0;");
    _client.println("}");
    
    _client.println(".show-password-label input[type=\"checkbox\"] {");
    _client.println("  width: 16px; height: 16px; margin-right: 6px;");
    _client.println("}");
    
    _client.println("input[type=\"color\"] {");
    _client.println("  width: 60px; height: 40px; padding: 2px;");
    _client.println("  border: 2px solid var(--border); border-radius: 8px;");
    _client.println("  cursor: pointer; transition: all 0.2s ease;");
    _client.println("}");
    
    _client.println("input[type=\"color\"]:hover {");
    _client.println("  border-color: var(--border-focus);");
    _client.println("}");
    
    // Range slider styling
    _client.println(".range-container {");
    _client.println("  display: flex; align-items: center; gap: 15px;");
    _client.println("}");
    
    _client.println("input[type=\"range\"] {");
    _client.println("  flex: 1; height: 6px; appearance: none; background: var(--border);");
    _client.println("  border-radius: 3px; outline: none;");
    _client.println("}");
    
    _client.println("input[type=\"range\"]::-webkit-slider-thumb {");
    _client.println("  appearance: none; width: 20px; height: 20px; background: var(--primary-color);");
    _client.println("  border-radius: 50%; cursor: pointer;");
    _client.println("}");
    
    _client.println(".range-value {");
    _client.println("  background: var(--primary-color); color: white; padding: 4px 12px;");
    _client.println("  border-radius: 12px; font-weight: 600; min-width: 40px; text-align: center;");
    _client.println("}");
    
    // Time input styling - constrain width for iOS Safari
    _client.println("input[type=\"time\"] {");
    _client.println("  width: 100%; max-width: 100%; box-sizing: border-box;");
    _client.println("  height: 48px; padding: 12px; font-size: 1.1rem;");
    _client.println("  border: 2px solid var(--border); border-radius: 8px;");
    _client.println("  background: var(--card-bg); outline: none;");
    _client.println("}");
    
    // Checkbox and radio styling
    _client.println(".checkbox-group, .radio-group {");
    _client.println("  margin-bottom: 4px;");
    _client.println("}");
    
    _client.println(".checkbox-label, .radio-label {");
    _client.println("  display: flex; align-items: center; cursor: pointer; font-size: 1.1rem;");
    _client.println("  font-weight: 500; color: var(--text-primary); padding: 2px 0;");
    _client.println("}");
    
    _client.println("input[type=\"checkbox\"], input[type=\"radio\"] {");
    _client.println("  width: 18px; height: 18px; margin-right: 12px; cursor: pointer;");
    _client.println("}");
    
    // Add the separator line above save button
    _client.println(".button-separator {");
    _client.println("  width: 100%; height: 1px;");
    _client.println("  background: var(--border);");
    _client.println("  margin: 30px 0 20px 0;");
    _client.println("}");
    
    _client.println(".save-button {");
    _client.println("  width: 100%; padding: 20px; font-size: 1.2rem; font-weight: 600;");
    _client.println("  color: white; background: linear-gradient(135deg, var(--success-color) 0%, #047857 100%);");
    _client.println("  border: none; border-radius: 12px; cursor: pointer;");
    _client.println("  transition: all 0.2s ease; margin-top: 20px; box-shadow: var(--shadow);");
    _client.println("}");
    
    _client.println(".save-button:hover {");
    _client.println("  transform: translateY(-2px); box-shadow: var(--shadow-lg);");
    _client.println("}");
    
    _client.println(".save-button:active { transform: translateY(0); }");
    
    _client.println(".success-message {");
    _client.println("  background: linear-gradient(135deg, #d1fae5 0%, #a7f3d0 100%);");
    _client.println("  color: #065f46; padding: 32px; border-radius: 12px; text-align: center;");
    _client.println("  font-size: 1.3rem; font-weight: 600; border: 2px solid #34d399;");
    _client.println("  animation: slideIn 0.3s ease;");
    _client.println("}");
    
    _client.println("@keyframes slideIn {");
    _client.println("  from { opacity: 0; transform: translateY(-20px); }");
    _client.println("  to { opacity: 1; transform: translateY(0); }");
    _client.println("}");
    
    _client.println("@media (max-width: 600px) {");
    _client.println("  body { padding: 10px; }");
    _client.println("  #header { font-size: 2rem; padding: 30px 20px; }");
    _client.println("  #inputs { padding: 20px; }");
    _client.println("}");
    
    // Inject custom CSS if provided
    if (_customCSS.length() > 0) {
        _client.println(_customCSS);
    }
    
    _client.println("</style>");

    _client.println("<title>" + _pageTitle + "</title>");
    _client.println("</head>");

    _client.println("<body>");
    _client.println("<div id=\"container\">");
    _client.println("<h1 id=\"header\">" + _pageTitle + "</h1>");
    _client.println("<div id=\"inputs\">");
}

/**
 * End HTML form output with JavaScript
 */
void FormBuilder::htmlEnd() {
    // Add the separator line before the save button
    _client.println("<div class=\"button-separator\"></div>");
    _client.println("<button type=\"button\" class=\"save-button\" onclick=\"SendText()\">Save Configuration</button>");
    _client.println("</div></div>");

    _client.println("<script>");

    // Range slider update function
    _client.println("function updateRangeValue(fieldId, value) {");
    _client.println("  document.getElementById(fieldId + '_value').textContent = value;");
    _client.println("}");

    // Password visibility toggle function
    _client.println("function togglePassword(fieldId) {");
    _client.println("  var field = document.getElementById(fieldId);");
    _client.println("  field.type = field.type === 'password' ? 'text' : 'password';");
    _client.println("}");

    // JavaScript SendText() function - compact generic loop
    _client.println("function SendText() {");
    _client.println("  var request = new XMLHttpRequest();");
    _client.println("  var sep = '__SEP__';");
    _client.println("  var netText = '?';");
    _client.println("  var first = true;");
    _client.println("  for (var i = " + String(START_FIELD_TAG + 1) + "; i <= " + String(_fieldTag) + "; i++) {");
    _client.println("    if (!first) netText += sep;");
    _client.println("    first = false;");
    _client.println("    var fieldId = 'x' + i;");
    _client.println("    var value = '';");
    _client.println("    var fieldFound = false;");
    _client.println("    var field = document.getElementById(fieldId);");
    _client.println("    if (field) {");
    _client.println("      fieldFound = true;");
    _client.println("      if (field.type === 'checkbox') {");
    _client.println("        value = field.checked ? 'true' : 'false';");
    _client.println("      } else {");
    _client.println("        value = field.value || '';");
    _client.println("      }");
    _client.println("    } else {");
    _client.println("      var rc = document.querySelector('input[name=\"group_' + fieldId + '\"]:checked');");
    _client.println("      if (rc) { value = rc.value; fieldFound = true; }");
    _client.println("    }");
    _client.println("    if (fieldFound) {");
    _client.println("      netText += fieldId + '=' + encodeURIComponent(value);");
    _client.println("    }");
    _client.println("  }");

    // Update UI (clear form and show success message)
    _client.println("  document.body.innerHTML = '';");
    _client.println("  document.body.style.cssText = 'font-family: -apple-system, BlinkMacSystemFont, \\'Segoe UI\\', Roboto, sans-serif; background: #ffffff; margin: 0; padding: 20px; color: #1e293b; line-height: 1.6;';");
    _client.println("  var successBox = document.createElement('div');");
    _client.println("  successBox.className = 'success-message';");
    _client.println("  successBox.textContent = '✓ Configuration Saved!';");
    _client.println("  document.body.appendChild(successBox);");

    // Send the AJAX request with collected data
    _client.println("  if (!netText.endsWith('?') && !netText.endsWith('&')) {");
    _client.println("    netText += '&';");
    _client.println("  }");
    _client.println("  var nocache = 'nocache=' + Math.random() * 1000000;");
    _client.println("  request.open('GET', '/ajax_inputs' + netText + nocache, true);");
    _client.println("  request.send(null);");
    _client.println("}");
    _client.println("</script>");

    _client.println("</body>");
    _client.println("</html>");
    _client.println();
}

/**
 * URL Decode function
 */
String FormBuilder::urlDecode(String input) {
    String decoded = "";
    char temp[] = "0x00";
    unsigned int len = input.length();
    unsigned int i = 0;

    while (i < len) {
        char c = input.charAt(i);
        if (c == '+') {
            decoded += ' ';
        } else if (c == '%' && i + 2 < len) {
            temp[2] = input.charAt(i + 1);
            temp[3] = input.charAt(i + 2);
            decoded += (char) strtol(temp, NULL, 16);
            i += 2;
        } else {
            decoded += c;
        }
        i++;
    }
    return decoded;
}

/**
 * Process parameters from HTTP request
 */
void FormBuilder::getParameters() {
    if (!_client) return;

    String requestLine = "";
    String firstLine = "";
    bool headersDone = false;
    bool handledRequest = false;

    while (_client.connected() && _client.available()) {
        requestLine = _client.readStringUntil('\n');
        requestLine.trim();

        if (requestLine.length() == 0) {
            headersDone = true;
            break;
        }

        if (firstLine == "") firstLine = requestLine;

        if (requestLine.startsWith("GET /ajax_inputs")) {
            handledRequest = true;

            int queryStart = requestLine.indexOf('?');
            int queryEnd = requestLine.indexOf(' ', queryStart);
            if (queryStart == -1 || queryEnd == -1) {
                break;
            }

            String queryString = requestLine.substring(queryStart + 1, queryEnd);
            const String sep = "__SEP__";
            const int sepLen = sep.length();

            int pos = 0;
            int nextSep;
            int fieldIndex = 1;

            while (pos < queryString.length() && fieldIndex <= _numberFields) {
                nextSep = queryString.indexOf(sep, pos);
                if (nextSep == -1) nextSep = queryString.length();

                String param = queryString.substring(pos, nextSep);
                pos = nextSep + sepLen;

                int equalSign = param.indexOf('=');
                if (equalSign == -1) {
                    fieldIndex++;
                    continue;
                }

                String fieldTag = param.substring(0, equalSign);
                String value = param.substring(equalSign + 1);

                // Strip nocache parameter from the last field value
                int nocachePos = value.indexOf("&nocache=");
                if (nocachePos != -1) {
                    value = value.substring(0, nocachePos);
                }
                
                value = urlDecode(value);
                value.trim();
                if (value == "%20") value = "";
                if (value == "(None)") value = "";

                // Convert hex color values to integer strings for consistency
                if (value.startsWith("#")) {
                    String hexValue = value.substring(1);
                    int colorInt = (int)strtol(hexValue.c_str(), NULL, 16);
                    value = String(colorInt);
                }

                // Convert time format (HH:MM) to integer for consistency
                if (value.length() >= 5 && value.charAt(2) == ':') {
                    String hourStr = value.substring(0, 2);
                    String minStr = value.substring(3, 5);
                    int hours = hourStr.toInt();
                    int minutes = minStr.toInt();
                    int timeInt = hours * 100 + minutes;
                    value = String(timeInt);
                }

                // Check if value changed from default
                bool valueChanged = false;
                if (fieldIndex > 0 && fieldIndex <= MAX_FORM_FIELDS) {
                    String defaultValue = _fieldDefaults[fieldIndex - 1];
                    valueChanged = (value != defaultValue);
                }

                // Call the callback function with field index, value, and change flag
                if (_callback) {
                    _callback(fieldIndex, value, valueChanged);
                }

                fieldIndex++;
            }

            // Call the form complete callback if set
            if (_formCompleteCallback) {
                _formCompleteCallback();
            }

            _client.println("HTTP/1.1 200 OK");
            _client.println("Content-Type: text/plain");
            _client.println();
            _client.println("Configuration saved successfully!");
            _client.stop();
            return;
        }
    }

    if (!handledRequest && headersDone) {
        // Only serve the form for root path requests.
        // Other requests (favicon.ico, etc) must not trigger htmlStart()
        // because it resets _numberFields, which is needed by the AJAX parser.
        if (firstLine.startsWith("GET /") && !firstLine.startsWith("GET /ajax")) {
            htmlStart();
            
            // Call user's form builder function to add all form fields
            if (_formBuilderCallback) {
                _formBuilderCallback();
            }
            
            htmlEnd();
            _client.flush();
            delay(3000);
            _client.stop();
        } else {
            // Non-root, non-ajax request — just close without touching state
            _client.println("HTTP/1.1 404 Not Found");
            _client.println("Connection: close");
            _client.println();
            _client.flush();
            _client.stop();
        }
    }
}