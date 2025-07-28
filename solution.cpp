#include <iostream>
#include <vector>
#include <string>
#include <fstream>
#include <map>
#include <algorithm>
#include <cmath>
#include <iomanip>

#include "json.hpp"

using json = nlohmann::json;

using namespace std;

struct Share {
    double x;
    double y;

    friend ostream& operator<<(ostream& os, const Share& s) {
        os << "(" << s.x << ", " << s.y << ")";
        return os;
    }
};

int lagrangeInterpolateAtZero(const vector<Share>& points) {
    int k = points.size();
    double secret = 0.0;

    for (int j = 0; j < k; ++j) {
        double xj = points[j].x;
        double yj = points[j].y;

        double lj_at_zero = 1.0;
        for (int i = 0; i < k; ++i) {
            if (i != j) {
                double xi = points[i].x;

                double denominator = (xj - xi);
                if (denominator == 0) {
                    throw runtime_error("Duplicate x-coordinate found in share set. Cannot interpolate.");
                }

                lj_at_zero *= (-xi) / denominator;
            }
        }
        secret += yj * lj_at_zero;
    }

    return static_cast<int>(round(secret));
}

void generateCombinations(const vector<Share>& allShares, int k, int startIdx,
                          vector<Share>& currentCombination,
                          vector<vector<Share>>& allCombinations) {

    if (currentCombination.size() == k) {
        allCombinations.push_back(currentCombination);
        return;
    }

    if (startIdx >= allShares.size() || 
        currentCombination.size() + (allShares.size() - startIdx) < k) {
        return;
    }

    currentCombination.push_back(allShares[startIdx]);
    generateCombinations(allShares, k, startIdx + 1, currentCombination, allCombinations);
    currentCombination.pop_back();

    generateCombinations(allShares, k, startIdx + 1, currentCombination, allCombinations);
}

string solveShamirWithOutliers(const string& jsonFilePath, int kValue) {
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

    vector<Share> allShares;
    for (json::iterator it = j.begin(); it != j.end(); ++it) {
        try {
            double x_val = stod(it.key()); 
            double y_val = it.value().get<double>(); 

            allShares.push_back({x_val, y_val});
        } catch (const exception& e) {
            cerr << "Warning: Could not parse share '" << it.key() << "': '" << it.value() << "'. Skipping. Error: " << e.what() << endl;
            continue;
        }
    }

    int n = allShares.size();
    cout << "DEBUG: Total shares (n): " << n << endl;
    cout << "DEBUG: Required shares (k): " << kValue << endl;

    if (n < kValue) {
        return "Error: Not enough shares (" + to_string(n) + ") to reconstruct the secret (need " + to_string(kValue) + ").";
    }

    vector<vector<Share>> allCombinations;
    vector<Share> currentCombination;
    generateCombinations(allShares, kValue, 0, currentCombination, allCombinations);

    cout << "DEBUG: Generated " << allCombinations.size() << " combinations." << endl;

    map<int, int> secretCounts;

    for (const auto& combo : allCombinations) {
        try {
            int secretCandidate = lagrangeInterpolateAtZero(combo);
            secretCounts[secretCandidate]++;
        } catch (const runtime_error& e) {
            cerr << "DEBUG: Skipping combination due to interpolation error: " << e.what() << endl;
        }
    }

    if (secretCounts.empty()) {
        return "Error: Could not determine secret. No valid combinations yielded a secret.";
    }

    int mostCommonSecret = 0;
    int maxCount = 0;
    for (const auto& pair : secretCounts) {
        if (pair.second > maxCount) {
            maxCount = pair.second;
            mostCommonSecret = pair.first;
        }
    }

    return to_string(mostCommonSecret);
}

int main() {
    json dummy_data_for_json;
    dummy_data_for_json["1"] = "12"; 
    dummy_data_for_json["2"] = "17"; 
    dummy_data_for_json["3"] = "24"; 
    dummy_data_for_json["4"] = "33"; 
    dummy_data_for_json["5"] = "44"; 
    dummy_data_for_json["6"] = "100";

    string json_file_name = "shares_input.json";

    try {
        ofstream ofs(json_file_name);
        ofs << setw(4) << dummy_data_for_json << endl;
        cout << "Created dummy JSON file: '" << json_file_name << "'" << endl;
    } catch (const exception& e) {
        cerr << "Error creating dummy JSON file: " << e.what() << endl;
        return 1;
    }

    int k_value_for_test = 3; 

    cout << "\nAttempting to find the secret using k=" << k_value_for_test << " shares..." << endl;

    string identified_secret_result = solveShamirWithOutliers(json_file_name, k_value_for_test);

    if (identified_secret_result.rfind("Error:", 0) == 0) {
        cerr << "\nFAILURE: " << identified_secret_result << endl;
        return 1;
    } else {
        cout << "\nSUCCESS! The identified secret is: " << identified_secret_result << endl;
        return 0;
    }
}
