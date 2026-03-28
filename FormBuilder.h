/**
 * FormBuilder.h - Generic HTML Form Builder Library for ESP32
 * 
 * A library for creating responsive web forms with WiFi, NTP,
 * and various configuration options for ESP32 devices.
 * 
 * Author: FormBuilder Library
 * License: MIT
 */

#ifndef FORMBUILDER_H
#define FORMBUILDER_H

#include <Arduino.h>
#include <WiFi.h>
#include <WiFiClient.h>
#include <WiFiServer.h>

// Maximum number of options per dropdown field
#ifndef MAX_FIELD_OPTIONS
#define MAX_FIELD_OPTIONS 50
#endif

// Maximum number of valid values per field
#ifndef MAX_VALID
#define MAX_VALID 10
#endif

// Maximum number of form fields
#ifndef MAX_FORM_FIELDS
#define MAX_FORM_FIELDS 100
#endif

/**
 * Callback function type for handling form data
 * @param fieldIndex The index of the form field (1-based)
 * @param value The string value received from the form
 * @param valueChanged True if value differs from the default, false if unchanged
 */
typedef void (*FormDataCallback)(int fieldIndex, String value, bool valueChanged);

/**
 * Callback function type for building the form
 * This function should call addText, addDropDown, etc. to build the form
 */
typedef void (*FormBuilderCallback)();

/**
 * Callback function type for when all form fields have been processed
 * Called after all individual field callbacks are complete
 */
typedef void (*FormCompleteCallback)();

/**
 * FormBuilder Class
 * 
 * Provides methods to create HTML form fields and handle form submissions
 */
class FormBuilder {
public:
    /**
     * Constructor
     */
    FormBuilder();

    /**
     * Initialize the form builder with a WiFi server
     * @param server Pointer to WiFiServer instance
     */
    void begin(WiFiServer* server);

    /**
     * Set the callback function for form data processing
     * @param callback Function to call when form data is received
     */
    void setCallback(FormDataCallback callback);

    /**
     * Set the callback function for building the form
     * @param callback Function that adds all the form fields
     */
    void setFormBuilder(FormBuilderCallback callback);

    /**
     * Set the callback function for when all form processing is complete
     * @param callback Function to call after all fields have been processed
     */
    void setFormCompleteCallback(FormCompleteCallback callback);

    /**
     * Set the page title displayed in browser tab and header
     * @param title The title to display
     */
    void setTitle(String title);

    /**
     * Add custom CSS to be injected into the page
     * @param css Custom CSS rules to add to the stylesheet
     */
    void addCustomCSS(String css);

    /**
     * Handle incoming client connections and form submissions
     * Call this in your main loop when form functionality is needed
     */
    void handleClient();

    /**
     * Clean up and free resources when form functionality no longer needed
     * Call this after configuration is complete to free memory
     */
    void cleanup();

    /**
     * Add a subheading to organize form sections
     * @param text The subheading text to display
     */
    void addSubheading(String text);

    /**
     * Add a text input field to the form
     * @param prompt Display label for the field
     * @param defaultValue Default text value
     */
    void addText(String prompt, String defaultValue);

    /**
     * Add a dropdown field with comma-separated options
     * @param prompt Display label for the field
     * @param options Comma-separated list of options
     * @param defaultIndex Index of default selected option (0-based)
     * @param returnText If true, returns option text; if false, returns index
     */
    void addDropDown(String prompt, String options, int defaultIndex, bool returnText = false);

    /**
     * Add a range dropdown (e.g., 0-23 for hours)
     * @param prompt Display label for the field
     * @param minVal Minimum value in range
     * @param maxVal Maximum value in range
     * @param defaultValue Default selected value
     */
    void addDropDownRange(String prompt, int minVal, int maxVal, int defaultValue);

    /**
     * Add a color picker field
     * @param prompt Display label for the field
     * @param defaultColor Default color as integer (e.g., 0xFF0000 for red)
     */
    void addColorPicker(String prompt, int defaultColor);

    /**
     * Add a number input field with range validation
     * @param prompt Display label for the field
     * @param minVal Minimum allowed value
     * @param maxVal Maximum allowed value
     * @param step Step increment (default 1)
     * @param defaultValue Default numeric value
     */
    void addNumber(String prompt, int minVal, int maxVal, int step, int defaultValue);

    /**
     * Add a range slider for numeric values
     * @param prompt Display label for the field
     * @param minVal Minimum value
     * @param maxVal Maximum value
     * @param step Step increment (default 1)
     * @param defaultValue Default slider value
     */
    void addRange(String prompt, int minVal, int maxVal, int step, int defaultValue);

    /**
     * Add a time picker input
     * @param prompt Display label for the field
     * @param defaultTime Default time as integer (e.g., 1356 for 13:56)
     * @param includeSeconds If true, includes seconds in time picker
     */
    void addTime(String prompt, int defaultTime, bool includeSeconds = false);

    /**
     * Add a password input field
     * @param prompt Display label for the field
     * @param defaultValue Default password value
     */
    void addPassword(String prompt, String defaultValue);

    /**
     * Add a checkbox input
     * @param prompt Display label for the checkbox
     * @param defaultChecked Default checked state
     */
    void addCheckbox(String prompt, bool defaultChecked);

    /**
     * Add a radio button group with comma-separated options
     * @param prompt Display label for the radio group
     * @param options Comma-separated list of options
     * @param defaultIndex Index of default selected option (0-based)
     * @param returnText If true, returns option text; if false, returns index
     */
    void addRadio(String prompt, String options, int defaultIndex, bool returnText = false);

    /**
     * Add a hidden field — occupies a field index but renders nothing visible.
     * Use to preserve field numbering when a preset slot is unused.
     * @param defaultValue Value returned on form submit
     */
    void addHidden(String defaultValue);

private:
    // Internal structure for field configuration
    struct FieldSettings {
        String fieldPrompt;
        String textDefault;
        byte numDefault;
        String heading;
        String fieldOptions[MAX_FIELD_OPTIONS];
        byte valid[MAX_VALID];
        bool returnPrompts;
        String returnVal;
        bool isRangeDropdown;
        int rangeMin;
        int rangeMax;
        int rangeDefault;
        bool isColorPicker;
        int colorDefault;
        // New fields for additional input types
        bool isNumberInput;
        int numberMin, numberMax, numberStep, numberDefault;
        bool isRangeSlider;
        int rangeStep;
        bool isTimeInput;
        bool timeIncludeSeconds;
        bool isPasswordInput;
        bool isCheckbox;
        bool checkboxDefault;
        bool isRadio;
        String radioGroup;
        String radioValue;
        bool radioSelected;
    };

    // Private member variables
    WiFiServer* _server;
    WiFiClient _client;
    FormDataCallback _callback;
    FormBuilderCallback _formBuilderCallback;
    FormCompleteCallback _formCompleteCallback;
    FieldSettings _settings;
    
    // Form generation state
    static const int START_FIELD_TAG = 10;
    int _fieldTag;
    int _numberFields;
    String _pageTitle;
    String _customCSS;
    
    // Storage for default values to detect changes
    String _fieldDefaults[MAX_FORM_FIELDS];

    // Private methods
    void clearSettings();
    void renderDropdown();
    void renderTextInput();
    void renderColorPicker();
    void renderSubheading(String text);
    void renderNumberInput();
    void renderRangeSlider();
    void renderTimeInput();
    void renderPasswordInput();
    void renderCheckbox();
    void renderRadio();
    void renderHidden();
    void htmlStart();
    void htmlEnd();
    void getParameters();
    String urlDecode(String input);
};

#endif // FORMBUILDERDEV_H