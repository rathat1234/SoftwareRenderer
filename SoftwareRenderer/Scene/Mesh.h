#pragma once
#include "../Math/Vec3.h"
#include <vector>
#include <fstream>
#include <sstream>
#include <string>
using namespace std;

// ============================================================
// Mesh.h - 3D 메시 데이터 및 OBJ 파일 로더
// ============================================================

class Mesh {
public:
    vector<Vec3> vertices;  // 버텍스 위치 배열
    vector<int>  indices;   // 삼각형 인덱스 배열 (3개씩 묶어서 삼각형 구성)

    // Wavefront OBJ 파일 파싱
    // - 'v' 라인: 버텍스 좌표 읽기
    // - 'f' 라인: 면 인덱스 읽기 (폴리곤 -> 삼각형 팬 분할)
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
                // 버텍스 좌표 파싱
                Vec3 v;
                ss >> v.x >> v.y >> v.z;
                vertices.push_back(v);
            }
            else if (token == "f") {
                // 면 파싱: "v/vt/vn" 형식에서 버텍스 인덱스만 추출
                vector<int> idx;
                string part;
                while (ss >> part) {
                    int i = stoi(part.substr(0, part.find('/')));
                    idx.push_back(i - 1);  // OBJ는 1-based 인덱스
                }
                // 폴리곤 -> 삼각형 팬 분할 (Fan Triangulation)
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