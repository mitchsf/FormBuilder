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
#define MAX_FIELD_OPTIONS 20
#endif

// Maximum number of valid values per field
#ifndef MAX_VALID
#define MAX_VALID 10
#endif

/**
 * Callback function type for handling form data
 * @param fieldIndex The index of the form field (1-based)
 * @param value The string value received from the form
 */
typedef void (*FormDataCallback)(int fieldIndex, String value);

/**
 * Callback function type for building the form
 * This function should call addText, addDropDown, etc. to build the form
 */
typedef void (*FormBuilderCallback)();

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
     * Set the page title displayed in browser tab and header
     * @param title The title to display
     */
    void setTitle(String title);

    /**
     * Handle incoming client connections and form submissions
     * Call this in your main loop when form functionality is needed
     */
    void handleClient();

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
    };

    // Private member variables
    WiFiServer* _server;
    WiFiClient _client;
    FormDataCallback _callback;
    FormBuilderCallback _formBuilderCallback;
    FieldSettings _settings;
    
    // Form generation state
    static const int START_FIELD_TAG = 10;
    int _fieldTag;
    int _numberFields;
    String _pageTitle;

    // Private methods
    void clearSettings();
    void renderDropdown();
    void renderTextInput();
    void renderColorPicker();
    void renderSubheading(String text);
    void htmlStart();
    void htmlEnd();
    void getParameters();
    String urlDecode(String input);
};

#endif // FORMBUILDER_H