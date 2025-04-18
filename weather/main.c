#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <jansson.h>
#include <curl/curl.h>
#include <locale.h>

#define BUFFER_SIZE 256
#define MAX_RESPONSE_SIZE 65536

typedef enum {
    UNITS_METRIC,    // km/h
    UNITS_IMPERIAL   // mph
} UnitSystem;

// func callback for writing data to a buf
static size_t write_callback(void *ptr, size_t size, size_t nmemb, void *userdata) {
    char *buffer = (char *)userdata;
    size_t bytes = size * nmemb;
    
    size_t current_length = strlen(buffer);
    if (current_length + bytes >= MAX_RESPONSE_SIZE - 1) {
        fprintf(stderr, "buffer overflow\n");
        return 0;
    }
    
    strncat(buffer, ptr, bytes);
    return bytes;
}

int main(int argc, char *argv[]) {
    if (argc < 2) {
        fprintf(stderr, "Waiting format: %s [city]\n", argv[0]);
        return 1;
    }

    setlocale(LC_ALL, "");
    char *locale = setlocale(LC_ALL, NULL);
    UnitSystem units = UNITS_METRIC;

    if (locale && (strstr(locale, "en_US") || strstr(locale, "en_GB") || strstr(locale, "en_LR") || strstr(locale, "en_MM"))) {
        units = UNITS_IMPERIAL;
        }

    char city[BUFFER_SIZE] = "";

    for (int i = 1; i < argc; i++) {

        if (strlen(city) + strlen(argv[i]) + 3 >= BUFFER_SIZE) {
            fprintf(stderr, "Error: the city lenght exceeds the allowed value\n");
            return 1;
        }

        strncat(city, argv[i], BUFFER_SIZE - strlen(city) - 1);
        if (i < argc - 1) {

            strncat(city, "%20", BUFFER_SIZE - strlen(city) - 1);
        }
    }

    char url[BUFFER_SIZE];
    snprintf(url, sizeof(url), "https://wttr.in/%s?format=j1", city);

    char response[MAX_RESPONSE_SIZE] = "";

    CURL *curl;
    CURLcode res;

    curl_global_init(CURL_GLOBAL_DEFAULT);
    curl = curl_easy_init();
    
    if (!curl) {
        fprintf(stderr, "Error: CURL not initialized\n");
        curl_global_cleanup();
        return 1;
    }
    
    curl_easy_setopt(curl, CURLOPT_URL, url);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_callback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, response);
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, 10L);
    
    // send request
    res = curl_easy_perform(curl);
    
    if (res != CURLE_OK) {
        fprintf(stderr, "query execution error: %s\n", curl_easy_strerror(res));
        curl_easy_cleanup(curl);
        curl_global_cleanup();
        return 1;
    }
    
    curl_easy_cleanup(curl);
    curl_global_cleanup();

    // parsing JSON
    json_t *root;
    json_error_t error;

    root = json_loads(response, 0, &error);
    if (!root) {
        fprintf(stderr, "Error parsing JSON: %s\n", error.text);
        return 1;
    }

    // check for error response
    json_t *error_obj = json_object_get(root, "error");
    if (error_obj && json_is_string(error_obj)) {
        fprintf(stderr, "Error API: %s\n", json_string_value(error_obj));
        json_decref(root);
        return 1;
    }

    // extraction of weather data
    json_t *current_condition = json_object_get(root, "current_condition");
    if (!current_condition || !json_is_array(current_condition) || json_array_size(current_condition) == 0) {
        fprintf(stderr, "Error: failed to get current weather conditions\n");
        json_decref(root);
        return 1;
    }
    
    json_t *current = json_array_get(current_condition, 0);
    if (!current) {
        fprintf(stderr, "Error: failed to get current weather conditions\n");
        json_decref(root);
        return 1;
    }
    
    // temperature
    const char *temp_c = NULL;
    json_t *temp_obj = json_object_get(current, "temp_C");
    if (temp_obj && json_is_string(temp_obj)) {
        temp_c = json_string_value(temp_obj);
    }
    
    // description of weather
    const char *desc = NULL;
    json_t *weather_desc = json_object_get(current, "weatherDesc");
    if (weather_desc && json_is_array(weather_desc) && json_array_size(weather_desc) > 0) {
        json_t *desc_obj = json_array_get(weather_desc, 0);
        if (desc_obj) {
            json_t *value_obj = json_object_get(desc_obj, "value");
            if (value_obj && json_is_string(value_obj)) {
                desc = json_string_value(value_obj);
            }
        }
    }
    
    // wind speed
    const char *wind_speed = NULL;
    json_t *wind_speed_obj = json_object_get(current, "windspeedKmph");
    if (wind_speed_obj && json_is_string(wind_speed_obj)) {
        wind_speed = json_string_value(wind_speed_obj);
    }
    
    const char *wind_dir = NULL;
    json_t *wind_dir_obj = json_object_get(current, "winddir16Point");
    if (wind_dir_obj && json_is_string(wind_dir_obj)) {
        wind_dir = json_string_value(wind_dir_obj);
    }

    const char *temp_min = temp_c;
    const char *temp_max = temp_c;
    
    json_t *weather = json_object_get(root, "weather");
    if (weather && json_is_array(weather) && json_array_size(weather) > 0) {
        json_t *today = json_array_get(weather, 0);
        if (today) {
            json_t *min_temp_obj = json_object_get(today, "mintempC");
            json_t *max_temp_obj = json_object_get(today, "maxtempC");
            
            if (min_temp_obj && json_is_string(min_temp_obj)) {
                temp_min = json_string_value(min_temp_obj);
            }
            
            if (max_temp_obj && json_is_string(max_temp_obj)) {
                temp_max = json_string_value(max_temp_obj);
            }
        }
    }

    printf("Weather forecast for %s:\n", city);
    printf("Description: %s\n", desc ? desc : "No data");
   
    if (units == UNITS_METRIC) {
        printf("Wind: %s, speed: %s km/h\n", 
               wind_dir ? wind_dir : "No data", 
               wind_speed ? wind_speed : "No data");
    } else {
        const char *wind_speed_mph = NULL;
        json_t *wind_speed_mph_obj = json_object_get(current, "windspeedMiles");
        if (wind_speed_mph_obj && json_is_string(wind_speed_mph_obj)) {
            wind_speed_mph = json_string_value(wind_speed_mph_obj);
        }
        printf("Wind: %s, speed: %s mph\n", 
               wind_dir ? wind_dir : "No data", 
               wind_speed_mph ? wind_speed_mph : "No data");
        }
    
    printf("Temperature: from %s°C to %s°C\n", 
           temp_min ? temp_min : "No data", 
           temp_max ? temp_max : "No data");

    // free memory
    json_decref(root);
    
    return 0;
}