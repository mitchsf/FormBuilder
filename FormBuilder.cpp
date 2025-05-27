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
    _fieldTag = START_FIELD_TAG;
    _numberFields = 0;
    _pageTitle = "Default Title";
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
 * Handle incoming client connections and form submissions
 */
void FormBuilder::handleClient() {
    if (!_server) return;
    
    _client = _server->accept();
    if (_client) {
        getParameters();
    }
}

/**
 * Set the page title displayed in browser tab and header
 */
void FormBuilder::setTitle(String title) {
    _pageTitle = title;
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

    if (_settings.heading != "") _client.println("<h2>" + _settings.heading + "</h2>");

    // Create field group container
    _client.println("<div class=\"field-group\">");
    _client.println("<label class=\"field-label\">" + _settings.fieldPrompt + "</label>");
    _client.println("<input type='color' id='" + fieldId + "' value='" + _settings.textDefault + "'>");
    _client.println("</div>");
}

/**
 * Start HTML form output
 */
void FormBuilder::htmlStart() {
    // Reset form generation state for fresh form
    _fieldTag = START_FIELD_TAG;
    _numberFields = 0;
    
    _client.println("HTTP/1.1 200 OK");
    _client.println("Content-type:text/html");
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
    
    _client.println(".field-group { margin-bottom: 24px; }");
    
    _client.println(".field-label {");
    _client.println("  display: block; font-size: 1.1rem; font-weight: 500;");
    _client.println("  color: var(--text-primary); margin-bottom: 8px;");
    _client.println("}");
    
    _client.println("input[type=\"text\"], input[type=\"password\"], select {");
    _client.println("  width: 100%; height: 48px; padding: 12px; font-size: 1.1rem;");
    _client.println("  border: 2px solid var(--border); border-radius: 8px;");
    _client.println("  background: var(--card-bg); transition: all 0.2s ease; outline: none;");
    _client.println("}");
    
    _client.println("input[type=\"text\"]:focus, input[type=\"password\"]:focus, select:focus {");
    _client.println("  border-color: var(--border-focus);");
    _client.println("  box-shadow: 0 0 0 3px rgba(59, 130, 246, 0.1);");
    _client.println("}");
    
    _client.println("input[type=\"color\"] {");
    _client.println("  width: 100%; height: 60px; padding: 4px;");
    _client.println("  border: 2px solid var(--border); border-radius: 8px;");
    _client.println("  cursor: pointer; transition: all 0.2s ease;");
    _client.println("}");
    
    _client.println("input[type=\"color\"]:hover {");
    _client.println("  border-color: var(--border-focus);");
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

    // JavaScript SendText() function - UNCHANGED LOGIC
    _client.println("function SendText() {");
    
    // Collect form data directly by ID while fields still have values
    _client.println("  var request = new XMLHttpRequest();");
    _client.println("  var sep = '__SEP__';");
    _client.println("  var netText = '?';");

    // Build query string with current form values using direct getElementById
    int tempFieldTag = START_FIELD_TAG;
    for (int fieldIndex = 1; fieldIndex <= _numberFields; fieldIndex++) {
        tempFieldTag++;
        String fieldId = "x" + String(tempFieldTag);
        if (fieldIndex > 1) _client.println("  netText += sep;");
        _client.println("  var field" + String(fieldIndex) + " = document.getElementById('" + fieldId + "');");
        _client.println("  if (field" + String(fieldIndex) + ") netText += '" + fieldId + "=' + encodeURIComponent(field" + String(fieldIndex) + ".value);");
    }

    // Update UI (clear form and show success message)
    _client.println("  document.body.innerHTML = '';");  // Clear EVERYTHING on the page
    _client.println("  document.body.style.cssText = 'font-family: -apple-system, BlinkMacSystemFont, \\'Segoe UI\\', Roboto, sans-serif; background: #ffffff; margin: 0; padding: 20px; color: #1e293b; line-height: 1.6;';");
    _client.println("  var successBox = document.createElement('div');");
    _client.println("  successBox.className = 'success-message';");
    _client.println("  successBox.textContent = '✓ Configuration Saved!';");
    _client.println("  document.body.appendChild(successBox);");

    // Send the AJAX request with collected data
    _client.println("  var nocache = '&nocache=' + Math.random() * 1000000;");
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
    bool headersDone = false;
    bool handledRequest = false;

    while (_client.connected() && _client.available()) {
        requestLine = _client.readStringUntil('\n');
        requestLine.trim();

        if (requestLine.length() == 0) {
            headersDone = true;
            break;
        }

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
                    continue;
                }

                String fieldTag = param.substring(0, equalSign);
                String value = param.substring(equalSign + 1);
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

                // Call the callback function with field index and value
                if (_callback) {
                    _callback(fieldIndex, value);
                }

                fieldIndex++;
            }

            _client.println("HTTP/1.1 200 OK");
            _client.println("Content-Type: text/plain");
            _client.println();
            _client.println("Saved; restarting...");
            _client.stop();
            delay(500);
            ESP.restart();
            return;
        }
    }

    if (!handledRequest && headersDone) {
        htmlStart();
        
        // Call user's form builder function to add all form fields
        // This is exactly like the original htmlData() call
        if (_formBuilderCallback) {
            _formBuilderCallback();
        }
        
        htmlEnd();
        _client.stop();
    }
}