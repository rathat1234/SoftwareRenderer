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
    vector<Vec3> vertices;   // 버텍스 위치 배열
    vector<Vec2> uvs;        // UV 좌표 배열
    vector<int>  indices;    // 버텍스 인덱스
    vector<int>  uvIndices;  // UV 인덱스

    // Wavefront OBJ 파일 파싱
    // - 'v'  라인: 버텍스 좌표
    // - 'vt' 라인: UV 좌표
    // - 'f'  라인: 면 인덱스 (v/vt/vn 형식)
    bool loadOBJ(const char* path) {
        vertices.clear();
        uvs.clear();
        indices.clear();
        uvIndices.clear();

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
            else if (token == "vt") {
                // UV 좌표 파싱
                Vec2 uv;
                ss >> uv.u >> uv.v;
                uvs.push_back(uv);
            }
            else if (token == "f") {
                // 면 파싱: "v/vt/vn" 형식에서 버텍스 + UV 인덱스 추출
                vector<int> vIdx, vtIdx;
                string part;
                while (ss >> part) {
                    istringstream ps(part);
                    string vi, vti;
                    getline(ps, vi, '/');   // 버텍스 인덱스
                    getline(ps, vti, '/');  // UV 인덱스

                    vIdx.push_back(stoi(vi) - 1);
                    if (!vti.empty())
                        vtIdx.push_back(stoi(vti) - 1);
                    else
                        vtIdx.push_back(0);
                }
                // 폴리곤 -> 삼각형 팬 분할
                for (int i = 1; i + 1 < (int)vIdx.size(); i++) {
                    indices.push_back(vIdx[0]);
                    indices.push_back(vIdx[i]);
                    indices.push_back(vIdx[i + 1]);

                    uvIndices.push_back(vtIdx[0]);
                    uvIndices.push_back(vtIdx[i]);
                    uvIndices.push_back(vtIdx[i + 1]);
                }
            }
        }
        return true;
    }
};