#include <iostream>
#include <vector>
#include <string>
#include <fstream>
#include <map>
#include <algorithm>
#include <cmath>
#include <iomanip>

#include "json.hpp"
#include "uint256_t.h"

using json = nlohmann::json;

using namespace std;

struct Share {
    int x;
    uint256_t y;

    friend ostream& operator<<(ostream& os, const Share& s) {
        os << "(" << s.x << ", " << s.y << ")";
        return os;
    }
};

uint256_t decodeBaseValue(const string& value_str, int base) {
    uint256_t result = 0;
    for (char c : value_str) {
        int digit;
        if (c >= '0' && c <= '9') {
            digit = c - '0';
        } else if (c >= 'a' && c <= 'f') {
            digit = 10 + (c - 'a');
        } else if (c >= 'A' && c <= 'F') {
            digit = 10 + (c - 'A');
        } else {
            throw runtime_error("Invalid character in base value string.");
        }

        if (digit >= base) {
            throw runtime_error("Digit '" + string(1, c) + "' is out of range for base " + to_string(base) + ".");
        }
        result = result * base + digit;
    }
    return result;
}

uint256_t lagrangeInterpolateAtZero(const vector<Share>& points) {
    int k = points.size();
    uint256_t secret = 0;

    for (int j = 0; j < k; ++j) {
        uint256_t xj = points[j].x;
        uint256_t yj = points[j].y;

        uint256_t numerator_prod = 1;
        uint256_t denominator_prod = 1;

        for (int i = 0; i < k; ++i) {
            if (i != j) {
                uint256_t xi = points[i].x;

                uint256_t term_numerator = 0 - xi;
                uint256_t term_denominator = xj - xi;

                if (term_denominator == 0) {
                    throw runtime_error("Duplicate x-coordinate found in share set. Cannot interpolate.");
                }
                
                numerator_prod *= term_numerator;
                denominator_prod *= term_denominator;
            }
        }
        
        secret += (yj * numerator_prod) / denominator_prod;
    }

    return secret;
}

string solvePolynomial(const string& jsonFilePath) {
    json j;
    ifstream ifs(jsonFilePath);

    if (!ifs.is_open()) {
        return "Error: JSON file not found at '" + jsonFilePath + "'";
    }

    try {
        ifs >> j;
    } catch (const json::parse_error& e) {
        return "Error: Malformed JSON file at '" + jsonFilePath + "'. Details: " + e.what();
    }

    int n = j["keys"]["n"].get<int>();
    int k = j["keys"]["k"].get<int>();

    cout << "DEBUG: Processing file: " << jsonFilePath << endl;
    cout << "DEBUG: Total roots (n): " << n << endl;
    cout << "DEBUG: Required roots (k): " << k << endl;

    vector<Share> allShares;
    for (json::iterator it = j.begin(); it != j.end(); ++it) {
        if (it.key() == "keys") {
            continue;
        }
        try {
            int x_val = stoi(it.key());
            string base_str = it.value()["base"].get<string>();
            string value_str = it.value()["value"].get<string>();

            int base = stoi(base_str);
            uint256_t y_val = decodeBaseValue(value_str, base);

            allShares.push_back({x_val, y_val});
        } catch (const exception& e) {
            cerr << "Warning: Could not parse share for key '" << it.key() << "'. Skipping. Error: " << e.what() << endl;
            continue;
        }
    }

    sort(allShares.begin(), allShares.end(), [](const Share& a, const Share& b) {
        return a.x < b.x;
    });

    vector<Share> k_shares;
    if (allShares.size() < k) {
        return "Error: Not enough valid shares parsed to meet k requirement for " + jsonFilePath;
    }
    for (int i = 0; i < k; ++i) {
        k_shares.push_back(allShares[i]);
    }

    try {
        uint256_t secret = lagrangeInterpolateAtZero(k_shares);
        return secret.str();
    } catch (const runtime_error& e) {
        return "Error during interpolation for " + jsonFilePath + ": " + e.what();
    }
}

int main() {
    json test_case_1_data;
    test_case_1_data["keys"] = {{"n", 4}, {"k", 3}};
    test_case_1_data["1"] = {{"base", "10"}, {"value", "4"}};
    test_case_1_data["2"] = {{"base", "2"}, {"value", "111"}};
    test_case_1_data["3"] = {{"base", "10"}, {"value", "12"}};
    test_case_1_data["6"] = {{"base", "4"}, {"value", "213"}};

    string json_file_name_1 = "testcase1.json";
    try {
        ofstream ofs(json_file_name_1);
        ofs << setw(4) << test_case_1_data << endl;
        cout << "Created dummy JSON file: '" << json_file_name_1 << "'" << endl;
    } catch (const exception& e) {
        cerr << "Error creating dummy JSON file 1: " << e.what() << endl;
        return 1;
    }

    json test_case_2_data;
    test_case_2_data["keys"] = {{"n", 10}, {"k", 7}};
    test_case_2_data["1"] = {{"base", "6"}, {"value", "13444211440455345511"}};
    test_case_2_data["2"] = {{"base", "15"}, {"value", "aed7015a346d63"}};
    test_case_2_data["3"] = {{"base", "15"}, {"value", "6aeeb69631c227c"}};
    test_case_2_data["4"] = {{"base", "16"}, {"value", "e1b5e05623d881f"}};
    test_case_2_data["5"] = {{"base", "8"}, {"value", "316034514573652620673"}};
    test_case_2_data["6"] = {{"base", "3"}, {"value", "2122212201122002221120200210011020220200"}};
    test_case_2_data["7"] = {{"base", "3"}, {"value", "20120221122211000100210021102001201112121"}};
    test_case_2_data["8"] = {{"base", "6"}, {"value", "20220554335330240002224253"}};
    test_case_2_data["9"] = {{"base", "12"}, {"value", "45153788322a1255483"}};
    test_case_2_data["10"] = {{"base", "7"}, {"value", "1101613130313526312514143"}};

    string json_file_name_2 = "testcase2.json";
    try {
        ofstream ofs(json_file_name_2);
        ofs << setw(4) << test_case_2_data << endl;
        cout << "Created dummy JSON file: '" << json_file_name_2 << "'" << endl;
    } catch (const exception& e) {
        cerr << "Error creating dummy JSON file 2: " << e.what() << endl;
        return 1;
    }

    cout << "\n--- Solving Test Case 1 ---" << endl;
    string secret1 = solvePolynomial(json_file_name_1);
    cout << "Secret for Test Case 1: " << secret1 << endl;

    cout << "\n--- Solving Test Case 2 ---" << endl;
    string secret2 = solvePolynomial(json_file_name_2);
    cout << "Secret for Test Case 2: " << secret2 << endl;
    
    return 0;
}
