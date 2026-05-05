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

struct Vec2 { float u, v; };  // UV 좌표 타입

class Mesh {
public:
    vector<Vec3> vertices;               // 버텍스 위치 배열
    vector<Vec2> uvs;                    // UV 좌표 배열
    vector<int>  indices;                // 버텍스 인덱스
    vector<int>  uvIndices;              // UV 인덱스
    std::vector<int> matIndices;         // 삼각형마다 머티리얼 인덱스
    std::vector<std::string> matNames;   // 머티리얼 이름 목록

    // Wavefront OBJ 파일 파싱
    // - 'v'  라인: 버텍스 좌표
    // - 'vt' 라인: UV 좌표
    // - 'f'  라인: 면 인덱스 (v/vt/vn 형식)
    bool loadOBJ(const char* path) {
        vertices.clear();
        uvs.clear();
        indices.clear();
        uvIndices.clear();
        matIndices.clear();
        matNames.clear();

        ifstream file(path);
        if (!file.is_open()) return false;

        int currentMat = 0; 

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
            else if (token == "vt") {
                Vec2 uv;
                ss >> uv.u >> uv.v;
                uvs.push_back(uv);
            }
            else if (token == "usemtl") {
                string matName;
                ss >> matName;
                auto it = std::find(matNames.begin(), matNames.end(), matName);
                if (it == matNames.end()) {
                    currentMat = (int)matNames.size();
                    matNames.push_back(matName);
                }
                else {
                    currentMat = (int)(it - matNames.begin());
                }
            }
            else if (token == "f") {
                vector<int> vIdx, vtIdx;
                string part;
                while (ss >> part) {
                    istringstream ps(part);
                    string vi, vti;
                    getline(ps, vi, '/');
                    getline(ps, vti, '/');

                    vIdx.push_back(stoi(vi) - 1);
                    if (!vti.empty())
                        vtIdx.push_back(stoi(vti) - 1);
                    else
                        vtIdx.push_back(0);
                }
                for (int i = 1; i + 1 < (int)vIdx.size(); i++) {
                    indices.push_back(vIdx[0]);
                    indices.push_back(vIdx[i]);
                    indices.push_back(vIdx[i + 1]);

                    uvIndices.push_back(vtIdx[0]);
                    uvIndices.push_back(vtIdx[i]);
                    uvIndices.push_back(vtIdx[i + 1]);

                    matIndices.push_back(currentMat);
                }
            }
        }
        return true;
    }
};