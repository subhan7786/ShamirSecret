#include <iostream>
#include <fstream>
#include <map>
#include <vector>
#include <cmath>
#include <iomanip>
#include "json.hpp"

using namespace std;
using json = nlohmann::json;

typedef long double ld;
typedef pair<ld, ld> Point;

ld convertToDecimal(const string& value, int base) {
    ld result = 0;
    for (char c : value) {
        int digit = isdigit(c) ? c - '0' : tolower(c) - 'a' + 10;
        result = result * base + digit;
    }
    return result;
}

ld lagrangeInterpolation(const vector<Point>& points) {
    ld c = 0;
    int k = points.size();
    for (int i = 0; i < k; ++i) {
        ld xi = points[i].first;
        ld yi = points[i].second;
        ld li = 1.0;
        for (int j = 0; j < k; ++j) {
            if (j != i) {
                ld xj = points[j].first;
                li *= (0 - xj) / (xi - xj);
            }
        }
        c += yi * li;
    }
    return c;
}

ld processJSON(const string& filename) {
    ifstream file(filename);
    if (!file.is_open()) {
        cerr << "Failed to open file: " << filename << endl;
        return -1;
    }

    json j;
    file >> j;

    int k = j["keys"]["k"];
    vector<Point> points;

    for (auto& el : j.items()) {
        if (el.key() == "keys") continue;

        int x = stoi(el.key());
        int base = stoi(el.value()["base"].get<string>());
        string val = el.value()["value"].get<string>();

        ld y = convertToDecimal(val, base);
        points.push_back({(ld)x, y});
    }

    vector<Point> selectedPoints(points.begin(), points.begin() + k);

    return lagrangeInterpolation(selectedPoints);
}

int main() {
    ld c1 = processJSON("testcase1.json");
    ld c2 = processJSON("testcase2.json");

    cout << fixed << setprecision(0);
    cout << "Secret from Test Case 1: " << c1 << endl;
    cout << "Secret from Test Case 2: " << c2 << endl;

    return 0;
}
