#include <iostream>
#include <vector>
#include <map>
#include <curl/curl.h>
#define UID "u-s4t2ud-74a69e79c9c4645ee4a7dcc45cf90936c77407828f476572c1a8ca9cd0986992"
#define SECRET "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"
#define BASE_URL "https://api.intra.42.fr"

#include <string>
#include <sstream>
#include <set>

std::map<std::string, std::string> parseJSON(const std::string& jsonData, int bytes) {
    std::map<std::string, std::string> keyValueMap;

    std::istringstream iss(jsonData);
    std::string line;
    while (std::getline(iss, line, ',') && bytes--) {
        // Find the position of the colon
        size_t colonPos = line.find(':');
        if (colonPos != std::string::npos) {
            // Extract the key and value
            std::string key = line.substr(1, colonPos - 2);
            std::string value = line.substr(colonPos + 2);

            // Remove double quotes from the value if present
            if (value.front() == '"' && value.back() == '"') {
                value = value.substr(1, value.length() - 2);
            }

            // Add the key-value pair to the map
            keyValueMap[key] = value;
        }
    }

    return keyValueMap;
}

size_t get_user_data(char *buffer, size_t itemSize, size_t nitems, void *ignorethis) {

    size_t bytes = itemSize * nitems;

    std::map<std::string, std::string> data;
    std::vector<std::string> vec;

    data = parseJSON(std::string((char *)buffer), 20);

    for(std::map<std::string, std::string>::iterator iter = data.begin() ; iter != data.end() ; iter++) {
        if (iter->first == "usual_full_name")
            vec.push_back(iter->second);
        else if (iter->first == "email")
            vec.push_back(iter->second);
    }

    for(std::vector<std::string>::iterator seter = vec.begin() ; seter != vec.end() ; seter++) {
        std::cout << *seter << std::endl;
    }

    return bytes;

}

void    api_call(std::string access_token, void *user) {

    CURL *curl = curl_easy_init();

    struct curl_slist *chunk = NULL;

    if (!curl) {
        std::cerr << "FAILED INITIALIZING CURL FOR FETCHING USER DATA" << std::endl;
        exit(0);
    }

    std::cout << access_token << std::endl;


    std::string data_endpoint = std::string(BASE_URL) + "/v2/users";
    data_endpoint += "/" + std::string((char *)user);

    std::string headers = "Authorization: " + access_token;
    chunk = curl_slist_append(chunk, headers.c_str());

    curl_easy_setopt(curl, CURLOPT_URL, data_endpoint.c_str());
    curl_easy_setopt(curl ,CURLOPT_HTTPHEADER, chunk);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, get_user_data);

    CURLcode result = curl_easy_perform(curl);

    if (result != CURLE_OK) {
        std::cerr << "FAILED FETCHING USER DATA" << std::endl;
    }

}

size_t perform_action(char *buffer, size_t itemSize, size_t nitems, void *ignorethis) {
    
    size_t bytes = itemSize * nitems;

    std::string data(buffer);
    std::map<std::string, std::string> value = parseJSON(data, 50);
    std::string access_token = "Bearer ";

    for(std::map<std::string, std::string>::iterator iter = value.begin() ; iter != value.end() ; iter++) {
        if (iter->first == "\"access_token") {
            std::cout << std::string(iter->second) << std::endl;
            access_token += iter->second.erase(iter->second.length() - 1);
        }
    }

    api_call(access_token, ignorethis);

    return bytes;

}

size_t process_dad_jokes(char *buffer, size_t itemSize, size_t nitems, void *ignorethis) {
    size_t bytes = itemSize * nitems;

    (char *)ignorethis = 'a';

    std::cout << (char *)ignorethis << std::endl;

    return bytes;
}

std::string dad_jokes() {

    CURL *curl = curl_easy_init();

    if (!curl) {
        std::cout << "FAILED INITIALIZING CURL" << std::endl;
        exit(0);
    }

    struct curl_slist *chunk = NULL;
    std::string headers = "Accept: text/plain";
    chunk = curl_slist_append(chunk, headers.c_str());

    char *test;

    // std::cout << "TEST ==> " << std::hex << test << std::endl;

    curl_easy_setopt(curl, CURLOPT_URL, "https://icanhazdadjoke.com/");
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, chunk);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, process_dad_jokes);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, test);

    CURLcode result = curl_easy_perform(curl);

    if (result != CURLE_OK)
        std::cerr << "ERROR FETCHING DAD JOKE" << std::endl;

    curl_easy_cleanup(curl);

    return std::string(test);

}

int main(int ac, char *av[]) {

    // if (ac > 1) {
    //     CURL *curl = curl_easy_init();

    //     if (!curl) {
    //         std::cout << "FAILED INITIALIZING CURL" << std::endl;
    //         exit(0);
    //     }

    //     std::string auth_url = std::string(BASE_URL) + "/oauth/token";

    //     curl_easy_setopt(curl, CURLOPT_URL, auth_url.c_str());
    //     curl_easy_setopt(curl, CURLOPT_POSTFIELDS, "grant_type=client_credentials&client_id=u-s4t2ud-74a69e79c9c4645ee4a7dcc45cf90936c77407828f476572c1a8ca9cd0986992&client_secret=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");
    //     curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, perform_action);
    //     curl_easy_setopt(curl, CURLOPT_WRITEDATA, av[1]);

    //     CURLcode result = curl_easy_perform(curl);

    //     if (result != CURLE_OK) {
    //         std::cout << "ERROR CURLING LINK" << std::endl;
    //     }

    //     curl_easy_cleanup(curl);
    // }
    dad_jokes();
    // std::cout << "JOKE: " << joke << std::endl;
    return (0);

}