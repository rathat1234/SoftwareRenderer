#pragma once
#include "../Math/Vec3.h"
#include <vector>
#include <fstream>
#include <sstream>
#include <string>
using namespace std;

class Mesh {
public:
    vector<Vec3> vertices;
    vector<int>  indices;

    bool loadOBJ(const char* path) {
        vertices.clear();
        indices.clear();

        ifstream file(path);
        if (!file.is_open()) return false;

        string line;
        while (getline(file, line)) {
            istringstream ss(line);
            string token;
            ss >> token;

            if (token == "v") {
                Vec3 v;
                ss >> v.x >> v.y >> v.z;
                vertices.push_back(v);
            }
            else if (token == "f") {
                vector<int> idx;
                string part;
                while (ss >> part) {
                    int i = stoi(part.substr(0, part.find('/')));
                    idx.push_back(i - 1);
                }
                for (int i = 1; i + 1 < (int)idx.size(); i++) {
                    indices.push_back(idx[0]);
                    indices.push_back(idx[i]);
                    indices.push_back(idx[i + 1]);
                }
            }
        }
        return true;
    }
};